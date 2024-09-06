#include "xUART.h"
#include "stm32l476xx.h"

volatile uint32_t Rx2_Counter=0;
uint8_t USART2_Buffer_Rx[BufferSize];

void uartConfig(void){
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


void USART2_IRQHandler(void){
	receive(USART2, USART2_Buffer_Rx);

}
/*void send(USART_TypeDef *USARTx, uint8_t *buffer){
		USARTx->CR1|=USART_CR1_TXEIE;
		USARTx->TDR=buffer[0];
}*/
void receive(USART_TypeDef *USARTx, uint8_t *buffer){
	if(USARTx->ISR&USART_ISR_RXNE){ //check event
		buffer[0]=USARTx->RDR;
		//this needs to be switched to a queue 
		switch(buffer[0]){
			case 'a':
				TIM4->ARR= 283;
				break;
			case 'b':
				TIM4->ARR= 254;
				break;
			case 'c':
				TIM4->ARR= 238;
				break;
			case 'd':
				TIM4->ARR= 212;
				break;
			case 'e':
				TIM4->ARR= 189;
				break;
			case 'f':
				TIM4->ARR=178 ;
				break;
			case 'g':
				TIM4->ARR=158;
				break;
			case 'h':
				TIM4->ARR= 141;
				break;
			default:
				TIM4->ARR= 283;
				break;
			}		//reading clears the rxne flag
		
	}
}
