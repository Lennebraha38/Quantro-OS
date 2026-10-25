/*
 * Quantro OS - VirtIO MMIO Tablet / Mouse Sürücüsü
 * AArch64 QEMU virt platformu üzerinde donanımsal mutlak (absolute) tablet ve fare desteği
 */
#include "mouse.h"
#include "gui.h"
#include "fb.h"
#include "uart.h"
#include "pmm.h"
#include "kernel.h"
#include "types.h"

#define VIRTIO_MMIO_BASE        0x0a000000ULL
#define VIRTIO_MMIO_STRIDE      0x200
#define VIRTIO_MMIO_MAX         32

#define MMIO_MAGIC              0x000
#define MMIO_VERSION            0x004
#define MMIO_DEVICE_ID          0x008
#define MMIO_HOST_FEATURES      0x010
#define MMIO_HOST_FEATURES_SEL  0x014
#define MMIO_GUEST_FEATURES     0x020
#define MMIO_GUEST_FEATURES_SEL 0x024
#define MMIO_GUEST_PAGE_SIZE    0x028
#define MMIO_QUEUE_SEL          0x030
#define MMIO_QUEUE_NUM_MAX      0x034
#define MMIO_QUEUE_NUM          0x038
#define MMIO_QUEUE_ALIGN        0x03c
#define MMIO_QUEUE_PFN          0x040
#define MMIO_QUEUE_READY        0x044
#define MMIO_QUEUE_NOTIFY       0x050
#define MMIO_STATUS             0x070

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTIO_ID_INPUT           18

#define INPUT_QSZ                 32

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} vring_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[INPUT_QSZ];
} vring_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} vring_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    vring_used_elem_t ring[INPUT_QSZ];
} vring_used_t;

typedef struct {
    vring_desc_t descs[INPUT_QSZ];
    vring_avail_t avail;
    uint8_t pad[4096 - sizeof(vring_desc_t)*INPUT_QSZ - sizeof(vring_avail_t)];
    vring_used_t used;
} vring_input_t;

static vring_input_t *s_vring = 0;
static struct virtio_input_event *s_events = 0;
static uintptr_t s_vring_phys = 0;
static uintptr_t s_events_phys = 0;

static uint64_t tablet_base = 0;
static uint16_t last_used_idx = 0;

static int cur_mouse_x = 640;
static int cur_mouse_y = 400;
static int cur_mouse_btn = 0;
static int pos_changed = 0;
static int btn_changed = 0;

static inline volatile uint32_t *mmio_ptr(uint64_t base, uint32_t off)
{
    return (volatile uint32_t *)(base + off);
}

static inline void mmio_write(uint64_t base, uint32_t off, uint32_t val)
{
    *mmio_ptr(base, off) = val;
}

static inline uint32_t mmio_read(uint64_t base, uint32_t off)
{
    return *mmio_ptr(base, off);
}

void mouse_init(uint64_t hhdm)
{
    tablet_base = 0;
    last_used_idx = 0;

    for (int i = 0; i < VIRTIO_MMIO_MAX; i++) {
        uint64_t base = hhdm + VIRTIO_MMIO_BASE + (uint64_t)i * VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_read(base, MMIO_MAGIC);
        if (magic != 0x74726976)
            continue;

        uint32_t dev = mmio_read(base, MMIO_DEVICE_ID);
        if (dev == VIRTIO_ID_INPUT) {
            tablet_base = base;
            uart_printf("[drv:mouse] VirtIO-Input aygiti bulundu: slot %d (MMIO: 0x%llx)\r\n",
                        i, (unsigned long long)base);

            /* Sayfa tahsisleri */
            s_vring_phys = pmm_alloc_page();
            s_events_phys = pmm_alloc_page();
            if (!s_vring_phys || !s_events_phys) {
                uart_puts("[drv:mouse] HATA: vring/events bellek tahsis edilemedi!\r\n");
                tablet_base = 0;
                return;
            }

            s_vring = (vring_input_t *)(hhdm + s_vring_phys);
            s_events = (struct virtio_input_event *)(hhdm + s_events_phys);
            memset(s_vring, 0, 4096);
            memset(s_events, 0, 4096);

            /* Aygıt Sıfırlama ve Tanıma */
            mmio_write(base, MMIO_STATUS, 0);
            mmio_write(base, MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

            /* Özellik Pazarlığı */
            mmio_write(base, MMIO_HOST_FEATURES_SEL, 0);
            (void)mmio_read(base, MMIO_HOST_FEATURES);
            mmio_write(base, MMIO_GUEST_FEATURES_SEL, 0);
            mmio_write(base, MMIO_GUEST_FEATURES, 0);
            mmio_write(base, MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

            /* Eventq (Kuyruk 0) Kurulumu */
            mmio_write(base, MMIO_QUEUE_SEL, 0);
            uint32_t max = mmio_read(base, MMIO_QUEUE_NUM_MAX);
            if (max == 0) {
                uart_puts("[drv:mouse] HATA: eventq kuyruk boyutu 0!\r\n");
                tablet_base = 0;
                return;
            }

            uint32_t qnum = (max < INPUT_QSZ) ? max : INPUT_QSZ;
            mmio_write(base, MMIO_QUEUE_NUM, qnum);
            mmio_write(base, MMIO_QUEUE_ALIGN, 4096);
            mmio_write(base, MMIO_GUEST_PAGE_SIZE, 4096);
            mmio_write(base, MMIO_QUEUE_PFN, (uint32_t)(s_vring_phys >> 12));
            mmio_write(base, MMIO_QUEUE_READY, 1);

            /* Olay halkasını boş alıcı tamponlarla doldur */
            s_vring->avail.idx = 0;
            s_vring->avail.flags = 0;
            s_vring->used.idx = 0;
            s_vring->used.flags = 0;

            for (uint32_t j = 0; j < qnum; j++) {
                s_vring->descs[j].addr = s_events_phys + (uint64_t)j * sizeof(struct virtio_input_event);
                s_vring->descs[j].len = sizeof(struct virtio_input_event);
                s_vring->descs[j].flags = 2; /* VIRTQ_DESC_F_WRITE: Aygıt yazar */
                s_vring->descs[j].next = 0;
                s_vring->avail.ring[j] = (uint16_t)j;
            }

            s_vring->avail.idx = (uint16_t)qnum;
            __sync_synchronize();

            /* Sürücü Tamamlandı Durumu */
            mmio_write(base, MMIO_STATUS,
                       VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                       VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

            /* Kuyruğu QEMU'ya bildir */
            mmio_write(base, MMIO_QUEUE_NOTIFY, 0);
            uart_puts("[drv:mouse] VirtIO Tablet & Fare basariyla etkinlestirildi!\r\n");
            return;
        }
    }

    uart_puts("[drv:mouse] VirtIO Tablet donanimi bulunamadi (UART emulasyonu aktif).\r\n");
}

void mouse_poll(void)
{
    if (!tablet_base || !s_vring || !s_events)
        return;

    int processed = 0;
    while (last_used_idx != s_vring->used.idx) {
        uint16_t ring_idx = last_used_idx % INPUT_QSZ;
        uint32_t desc_id = s_vring->used.ring[ring_idx].id;

        if (desc_id < INPUT_QSZ) {
            struct virtio_input_event *ev = &s_events[desc_id];

            if (ev->type == 3) { /* EV_ABS: Mutlak Koordinatlar (Tablet / Dokunmatik) */
                uint32_t sw = fb_width();
                uint32_t sh = fb_height();
                if (sw == 0) sw = 1280;
                if (sh == 0) sh = 800;

                if (ev->code == 0) { /* ABS_X */
                    cur_mouse_x = (int)(((uint64_t)ev->value * sw) / 32767);
                    pos_changed = 1;
                } else if (ev->code == 1) { /* ABS_Y */
                    cur_mouse_y = (int)(((uint64_t)ev->value * sh) / 32767);
                    pos_changed = 1;
                }
            } else if (ev->type == 1) { /* EV_KEY: Tus ve Tiklama */
                if (ev->code == 0x110 || ev->code == 0x14a) { /* BTN_LEFT / BTN_TOUCH */
                    cur_mouse_btn = ev->value ? 1 : 0;
                    btn_changed = 1;
                } else if (ev->code == 0x111) { /* BTN_RIGHT */
                    cur_mouse_btn = ev->value ? 2 : 0;
                    btn_changed = 1;
                }
            } else if (ev->type == 0) { /* EV_SYN: Senkronizasyon Olayi */
                if (pos_changed || btn_changed) {
                    gui_handle_mouse(cur_mouse_x, cur_mouse_y, cur_mouse_btn);
                    pos_changed = 0;
                    btn_changed = 0;
                }
            }

            /* Tamponu tekrar kuyruğa al */
            s_vring->avail.ring[s_vring->avail.idx % INPUT_QSZ] = (uint16_t)desc_id;
            __sync_synchronize();
            s_vring->avail.idx++;
            processed++;
        }
        last_used_idx++;
    }

    if (processed > 0) {
        mmio_write(tablet_base, MMIO_QUEUE_NOTIFY, 0);
    }
}
