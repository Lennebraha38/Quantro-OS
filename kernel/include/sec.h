#ifndef QUANTRO_SEC_H
#define QUANTRO_SEC_H

#include "types.h"

typedef struct {
    int  tpm_present;
    int  tpm_version_major;
    int  tpm_version_minor;
    int  secure_boot_enabled;
    int  vbs_enabled;          /* Virtualization Based Security */
    int  hvci_enabled;         /* Hypervisor Enforced Code Integrity */
    int  defender_active;
    int  threats_blocked;
} sec_status_t;

void sec_init(void);
void sec_print_status(void);
void sec_tpm_info(void);
int  sec_defender_scan(const char *target);
const sec_status_t *sec_get_status(void);

#endif /* QUANTRO_SEC_H */
