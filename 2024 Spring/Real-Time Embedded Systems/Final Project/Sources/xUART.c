#include "xUART.h"
#include "stm32l476xx.h"
#include "queue.h"
volatile uint32_t Rx2_Counter=0;
uint8_t USART2_Buffer_Rx[BufferSize];
extern QueueHandle_t uart_mailbox;
/* based on which note we have, we'll need to change either the PSC value or the ARR value.
	note values:
	a-220Hz , ARR: 283 
	b-246.94, ARR: 252 ______
	c-261.63, ARR: 238 
	d-293.66, ARR: 212
	e-329.63, ARR: 189
	f-349.23, ARR: 178 
	g-392, 	  ARR: 158 
	a4-440Hz  ARR: 141
*/
void uart2Config(void){
	//PA2 and PA3
	GPIOA->MODER&=0xFFFFFF0F;
	GPIOA->MODER|=0xA0;
	
	//Change AFR
	GPIOA->AFR[0]|=0x77<<8;
	
	//change output speed
	GPIOA->OSPEEDR|=0x000000F0;
	
	GPIOA->PUPDR&=0xFFFFFF0F;
	GPIOA->PUPDR&=0x50;
	//change output type
	//GPIOA->OTYPER&=~(0x3<<6);
	
	//Enable clock to UART
	RCC->APB1ENR1|=RCC_APB1ENR1_USART2EN;
	
	//turn on HSI clock
	RCC->CCIPR&=~RCC_CCIPR_USART2SEL;
	RCC->CCIPR|=RCC_CCIPR_USART2SEL_0;
	

	//turn on data length (8 bits), 1 stop bit, no parity bit, 
	USART2->CR1 &= ~USART_CR1_UE;
	
	USART2->CR1 &= ~USART_CR1_M;
	
	USART2->CR2 &= ~USART_CR2_STOP;
	
	USART2->CR1 &= ~USART_CR1_PCE;
	
	USART2->CR1 &= ~USART_CR1_OVER8;
	
	USART2->BRR = 0X0683;
	
	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
	//receive register not empty interrupt, receive enable, enable UART
	USART2->CR1|=(USART_CR1_RE|USART_CR1_TE);
	USART2->CR1|=USART_CR1_RXNEIE; 
	USART2->CR1&=~USART_CR1_TXEIE;
	NVIC_SetPriority(USART2_IRQn, 0); //not highest priority
	NVIC_EnableIRQ(USART2_IRQn); //enable interrupt
	
	USART2->CR1|=USART_CR1_UE;
	while((USART2->ISR&USART_ISR_REACK)==0);
	while((USART2->ISR&USART_ISR_TEACK)==0);
}


void uart4Config(void){
	//PA0 and PA1 (TX and RX respectively)
	GPIOA->MODER&=0xFFFF0FFF;
	GPIOA->MODER|=0xA000

	//Change AFR- may not be right? 
	GPIOA->AFR[0]|=0x77<<8;
	
	//change output speed
	GPIOA->OSPEEDR|=0x0000F000;
	
	GPIOA->PUPDR&=0xFFFF0FFF;
	GPIOA->PUPDR&=0x5000;
	RCC->APB1ENR1|=RCC_APB1ENR1_UART4EN;
	RCC->CCIPR&=~(RCC_CCIPR_UART4SEL);
	RCC->|=RCC_CCIPR_UART4SEL_0;
	
	//turn on data length (8 bits), 1 stop bit, no parity bit, 
	UART4->CR1 &= ~USART_CR1_UE;
	
	UART4->CR1 &= ~USART_CR1_M;
	
	UART4->CR2 &= ~USART_CR2_STOP;
	
	UART4->CR1 &= ~USART_CR1_PCE;
	
	UART4->CR1 &= ~USART_CR1_OVER8;
	
	UART4->BRR = 0X0683;
	
	UART4->CR1 |= (USART_CR1_TE | USART_CR1_RE);
	//receive register not empty interrupt, receive enable, enable UART
	UART4->CR1|=(USART_CR1_RE|USART_CR1_TE);
	UART4->CR1|=USART_CR1_RXNEIE; 
	UART4->CR1&=~USART_CR1_TXEIE;
	NVIC_SetPriority(UART4_IRQn, 0); //not highest priority
	NVIC_EnableIRQ(UART4_IRQn); //enable interrupt
	
	UART4->CR1|=USART_CR1_UE;
	while((UART4->ISR&USART_ISR_REACK)==0);
	while((UART4->ISR&USART_ISR_TEACK)==0);
}


void uart1Config(void){
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;  // enable gpio clock
	
	// PB7 is RX , PB6 is TX
	//PB6 and PB7
	GPIOB->MODER&=0xFFFF0FFF;
	GPIOB->MODER|=0xA000;
	
	//Change AFR-this may not be right
	//GPIOB->AFR[0]|=0x77<<8;
	GPIOB->AFR[0]|= 0x77 << (4*6); //Set the Alt Function for usart 1 on PB6 AND PB7

	//change output speed
	GPIOB->OSPEEDR|=0x0000F000;
	
	GPIOB->PUPDR&=0xFFFF0FFF;
	GPIOB->PUPDR&=0x5000;
	//change output type
	//GPIOA->OTYPER&=~(0x3<<6);
	
	//Enable clock to UART
	RCC->APB2ENR|=RCC_APB2ENR_USART1EN;
	
	//turn on HSI clock
	RCC->CCIPR&=~RCC_CCIPR_USART1SEL;
	RCC->CCIPR|=RCC_CCIPR_USART1SEL_0;
	

	//turn on data length (8 bits), 1 stop bit, no parity bit, 
	USART1->CR1 &= ~USART_CR1_UE; // disable usart1
	
	USART1->CR1 &= ~USART_CR1_M; // data length = 8 bits
	
	USART1->CR2 &= ~USART_CR2_STOP;// 1 stop bit
	
	USART1->CR1 &= ~USART_CR1_PCE; // no parity
	
	USART1->CR1 &= ~USART_CR1_OVER8; // oversample by 16
	
	USART1->BRR = 0X0683; // 9600 baud using HSI 16MHZ
	
	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE); // enable TX AND RX
	/*
	//receive register not empty interrupt, receive enable, enable UART
	USART1->CR1|=(USART_CR1_RE|USART_CR1_TE);
	USART1->CR1|=USART_CR1_RXNEIE; 
	USART1->CR1&=~USART_CR1_TXEIE;
	NVIC_SetPriority(USART1_IRQn, 0); //not highest priority
	NVIC_EnableIRQ(USART1_IRQn); //enable interrupt
	*/
	USART1->CR1|=USART_CR1_UE; // enable usart1
	while((USART1->ISR&USART_ISR_REACK)==0);// usart ready for TX
	while((USART1->ISR&USART_ISR_TEACK)==0);// usart ready for RX
}



/*
//original config
void uart1Config(void){
	//PB6 and PB7
	GPIOB->MODER&=0xFFFF0FFF;
	GPIOB->MODER|=0xA000;
	
	//Change AFR-this may not be right
	GPIOB->AFR[0]|=0x77<<8;
	
	//change output speed
	GPIOB->OSPEEDR|=0x0000F000;
	
	GPIOB->PUPDR&=0xFFFF0FFF;
	GPIOB->PUPDR&=0x5000;
	//change output type
	//GPIOA->OTYPER&=~(0x3<<6);
	
	//Enable clock to UART
	RCC->APB2ENR|=RCC_APB2ENR_USART1EN;
	
	//turn on HSI clock
	RCC->CCIPR&=~RCC_CCIPR_USART1SEL;
	RCC->CCIPR|=RCC_CCIPR_USART1SEL_0;
	

	//turn on data length (8 bits), 1 stop bit, no parity bit, 
	USART1->CR1 &= ~USART_CR1_UE;
	
	USART1->CR1 &= ~USART_CR1_M;
	
	USART1->CR2 &= ~USART_CR2_STOP;
	
	USART1->CR1 &= ~USART_CR1_PCE;
	
	USART1->CR1 &= ~USART_CR1_OVER8;
	
	USART1->BRR = 0X0683;
	
	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);
	//receive register not empty interrupt, receive enable, enable UART
	USART1->CR1|=(USART_CR1_RE|USART_CR1_TE);
	USART1->CR1|=USART_CR1_RXNEIE; 
	USART1->CR1&=~USART_CR1_TXEIE;
	NVIC_SetPriority(USART1_IRQn, 0); //not highest priority
	NVIC_EnableIRQ(USART1_IRQn); //enable interrupt
	
	USART1->CR1|=USART_CR1_UE;
	while((USART1->ISR&USART_ISR_REACK)==0);
	while((USART1->ISR&USART_ISR_TEACK)==0);
}
*/


void USART2_IRQHandler(void){
	uint8_t buffer;
	//For this function we'll need to convert distance to sound
	//The closer the distance, the higher the pitch
	if(USARTx->ISR&USART_ISR_RXNE){ //check event
		distance=USARTx->RDR;
		xTaskNotify(gatekeeperTaskHandle, &distance, eSetValueWithOverwrite);
		//xQueueSendToBackFromISR(uart_mailbox, &distance, pdFALSE);
		switch(distance){
			case 0:
				TIM4->ARR=477;
				break;
			case 1:
				TIM4->ARR=450;
				break;
			case 2:
				TIM4->ARR=425;
				break;
			case 3:
				TIM4->ARR=400;
				break;
			case 4:
				TIM4->ARR=378;
				break;
			case 5:
				TIM4->ARR=357;
				break;
			case 6:
				TIM4->ARR=337;
				break;
			case 7:
				TIM4->ARR=318;
				break;
			case 8:
				TIM4->ARR=300;
				break;
			case 9:
				TIM4->ARR=283
				break;
			case 10:
				TIM4->ARR=267;
				break;
			case 11:
				TIM4->ARR=254;
				break;
			case 12:
				TIM4->ARR=238;
				break;
			case 13:
				TIM4->ARR=224;
				break;
			case 14:
				TIM4->ARR=212;
				break;
			case 15:
				TIM4->ARR=200;
				break;
			case 16:
				TIM4->ARR=189;
				break;
			case 17:
				TIM4->ARR=178;
				break;
			case 18:
				TIM4->ARR=168;
				break;
			case 19:
				TIM4->ARR=158;
				break;
			case 20:
				TIM4->ARR=149;
				break;
			case 21:
				TIM4->ARR=141;
				break;
			}		//reading clears the rxne flag
		
	}
}

int distanceToNote(float distance) {
    if (distance >= 0 && distance < 2) {
        return 0;
    } else if (distance >= 2 && distance < 4) {
        return 1;
    } else if (distance >= 4 && distance < 6) {
        return 2;
    } else if (distance >= 6 && distance < 8) {
        return 3;
    } else if (distance >= 8 && distance < 10) {
        return 4;
    } else if (distance >= 10 && distance < 12) {
        return 5;
    } else if (distance >= 12 && distance < 14) {
        return 6;
    } else if (distance >= 14 && distance < 16) {
        return 7;
    } else if (distance >= 16 && distance < 18) {
        return 8;
    } else if (distance >= 18 && distance < 20) {
        return 9;
    } else if (distance >= 20 && distance < 22) {
        return 10;
    } else if (distance >= 22 && distance < 24) {
        return 11;
    } else if (distance >= 24 && distance < 26) {
        return 12;
    } else if (distance >= 26 && distance < 28) {
        return 13;
    } else if (distance >= 28 && distance < 30) {
        return 14;
    } else if (distance >= 30 && distance < 32) {
        return 15;
    } else if (distance >= 32 && distance < 34) {
        return 16;
    } else if (distance >= 34 && distance < 36) {
        return 17;
    } else if (distance >= 36 && distance < 38) {
        return 18;
    } else if (distance >= 38 && distance < 40) {
        return 19;
    } else if (distance >= 40 && distance <= 42) {
        return 20;
    } else {
        return -1; // Invalid distance
    }
}
