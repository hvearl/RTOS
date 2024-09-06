#ifndef xUART_H
#define xUART_H

#define BufferSize 32
#include "stm32l476xx.h"
#include "FreeRTOS.h"


void uart2Config(void);
void uart1Config(void);
void uart3Config(void);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);

void send(USART_TypeDef *USARTx);
void receive(USART_TypeDef *USARTx);

#endif
