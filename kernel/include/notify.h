#ifndef QUANTRO_NOTIFY_H
#define QUANTRO_NOTIFY_H

#include "types.h"

#define MAX_NOTIFICATIONS 8

typedef struct {
    char title[32];
    char msg[64];
    uint32_t timestamp;
} notify_item_t;

void notify_init(void);
void notify_push(const char *title, const char *msg);
void notify_list(void);
void notify_clear(void);

#endif /* QUANTRO_NOTIFY_H */
