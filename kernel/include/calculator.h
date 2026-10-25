#ifndef QUANTRO_CALCULATOR_H
#define QUANTRO_CALCULATOR_H

#include "types.h"

void calculator_init(void);
void calculator_run(void);
int  calculator_calc(int a, char op, int b, int *res);

#endif /* QUANTRO_CALCULATOR_H */
