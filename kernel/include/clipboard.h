#ifndef QUANTRO_CLIPBOARD_H
#define QUANTRO_CLIPBOARD_H

#include "types.h"

void clipboard_init(void);
int  clip_copy(const char *text);
const char *clip_paste(void);
void clip_status(void);

#endif /* QUANTRO_CLIPBOARD_H */
