#ifndef xTIM_H
#define xTIM_H
#include "stm32l476xx.h"

extern unsigned int lookup_index;

void tim4Config(void);
void TIM4_IRQHandler(void);



#endif
