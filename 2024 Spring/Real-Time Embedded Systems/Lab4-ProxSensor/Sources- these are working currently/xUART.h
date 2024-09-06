#ifndef xUART_H
#define xUART_H

#define BufferSize 32
#include "stm32l476xx.h"


void uart2Config(void);
void uart1Config(void);
void USART2_IRQHandler(void);
void receive(USART_TypeDef *USARTx, uint8_t *buffer);
void send(USART_TypeDef *USARTx, uint8x4_t *buffer);

#endif
