#ifndef QUANTRO_HEAP_H
#define QUANTRO_HEAP_H

#include "types.h"

void  heap_init(uintptr_t base_vaddr, size_t initial_size);
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void  kfree(void *ptr);
size_t heap_get_used(void);
size_t heap_get_total(void);

#endif /* QUANTRO_HEAP_H */
