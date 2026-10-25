#ifndef QUANTRO_SOUND_H
#define QUANTRO_SOUND_H

#include "types.h"

void sound_init(void);
void sound_startup(void);
void sound_beep(uint32_t freq, uint32_t duration_ms);
void sound_set_volume(int vol);
int  sound_get_volume(void);

#endif /* QUANTRO_SOUND_H */
