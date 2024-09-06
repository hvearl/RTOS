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
void USART2_IRQHandler(void){
	receive(USART2);
	send(USART2);
}
void receive(USART_TypeDef *USARTx){
	uint8_t buffer;
	if(USARTx->ISR&USART_ISR_RXNE){ //check event
		buffer=USARTx->RDR;
		xQueueSendToBackFromISR(uart_mailbox, &buffer, pdFALSE);
		switch(buffer){
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
void send(USART_TypeDef *USARTx){
	uint8_t buffer;
	if(xQueueReceive(uart_mailbox,&buffer,0)==pdTRUE){
		if(USARTx->ISR&USART_ISR_TXE){
				USARTx->TDR=buffer&0xFF;
		}
	}	
	else{
				USARTx->CR1&=~USART_CR1_TXEIE;
	}
}