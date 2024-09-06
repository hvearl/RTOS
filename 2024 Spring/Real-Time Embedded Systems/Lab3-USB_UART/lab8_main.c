#include "stm32l476xx.h"

void USART_Write (USART_TypeDef *USARTx, uint8_t * buffer, uint32_t nBytes) 
{

    int i;

    for (i=0; i < nBytes; i++) 
	  {
        while (!(USARTx->ISR & USART_ISR_TXE));
        USARTx->TDR = buffer[i] & 0xFF;
    }

    while (!(USARTx->ISR & USART_ISR_TC));

    USARTx->ICR |= USART_ICR_TCCF;
}

void USART_Read(USART_TypeDef *USARTx, uint8_t *buffer, uint32_t nBytes)
{
	unsigned char write[5] = {0, 0, 'C', 13, '\n'};
	
	for(int i=0; i < nBytes; i++)
	{
		while(!(USARTx->ISR & USART_ISR_RXNE));
		buffer[i] = USARTx->RDR;
	}
	
	if(buffer[0] == 't' || buffer[0] == 'T')
	{
		ADC1->CR |= ADC_CR_ADSTART;
		while((ADC123_COMMON->CSR && ADC_CSR_EOC_MST) ==0);
		float temp = 80.0/(0x512-0x410)*(ADC1->DR-0x410)+30.0;
		int temp1 = (int)temp % 10;
		int temp2 = ((int)temp % 100-temp1)/10;
		write[1] = temp1 + 48;
		write[0] = temp2 + 48;
		
		USART_Write(USARTx, write, 5);
	}
}

void USART_Init (USART_TypeDef * USARTx)
{
	USARTx->CR1 &= ~USART_CR1_UE;
	
	USARTx->CR1 &= ~USART_CR1_M;
	
	USARTx->CR2 &= ~USART_CR2_STOP;
	
	USARTx->CR1 &= ~USART_CR1_PCE;
	
	USARTx->CR1 &= ~USART_CR1_OVER8;
	
	USARTx->BRR = 0X0683;
	
	USARTx->CR1 |= (USART_CR1_TE | USART_CR1_RE);
	
	USARTx->CR1 |= USART_CR1_UE;
	
	while ((USARTx->ISR & USART_ISR_TEACK) == 0);

	while ((USARTx->ISR & USART_ISR_REACK) ==0);
}

int main(void)
{
	// ENABLE THE HSI
	RCC->CR |= RCC_CR_HSION; 
	while((RCC->CR & RCC_CR_HSIRDY) == 0);
	RCC->CFGR |= 0x00000001;
	
	

	//EABLE ADC CLOCK BIT
	RCC->AHB2ENR |= 0x2000;
	
	//DISABLE ADC1 BY CLEARING ADEN BIT
	ADC1->CR &= 0x0;
	
	//ENABLE I/O ANALOG SWITCH VOLTAGE BOOSTER
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN;
	
	//ENABLE CONVERSION OF INTERNAL CHANNELS. CLOCK NOT DIVIDED 
	//ALREADY SET UP. CLOCK MODE SET TO SYNCHRONOUS CLOCK MODE 
	//HCLK/1. ADC IN INDEPENDENT MODE
	ADC123_COMMON->CCR |= 0x410000;
	
	//WAKE FROM DEEP POWER DOWN. 
	int wait_time = 20*(16000000/1000000);
	
	//DEEPPWD = 0: ADC NOT IN DEEP POWER DOWN
	//DEEPPWD = 1: ADC IN DEEP POWER DOWN (DEFAULT)
	if((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)
	{
		ADC1->CR &= ~ADC_CR_DEEPPWD; //EXIT DEEP POWER MODE
	}
	
	//ENABLE THE ADC INTENAL VOLTAGE REGULATOR
	ADC1->CR |= ADC_CR_ADVREGEN;
	
	//WAIT FOR START UP OF VOLTAGE REGULATOR. 
	while(wait_time != 0)
	{
		wait_time--;
	}
	
	//REGISTER ALREADY SET FOR 12 BITS. ALREADY SET TO RIGHT 
	//SET TO CHANNEL 17 AS 1ST CONVERSION
	ADC1->SQR1 |= 0x440;
	
	//SET CHANNEL 17 TO SINGLE ENDED INPUT MODE (ALREADY DONE)
	//SET SAMPLE TIME TO 24.5 SAMPLE/S
	ADC1->SMPR1 =0x3;
	
	//ADC SET TO DISCONTINUOUS MODE. SELECT SOFTWARE TRIGGERS. 
	ADC1->CFGR &= ~(ADC_CFGR_CONT);
	ADC1->CFGR &= ~(ADC_CFGR_EXTEN);
	
	//ENABLE ADC1
	ADC1->CR |= 0x1;
	
	//WAIT FOR ADC1 READY
	//int temp = ADC1->ISR;
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0);
	
	//SET GPIOA PIN 2 AND 3
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER &= 0xFFFFFF0F;
	GPIOA->MODER |= 0xA0;
	
	GPIOA->AFR[0] |= 0x77 << 8;
	GPIOA->OSPEEDR |= 0x000000F0;
	GPIOA->PUPDR &= 0xFFFFFF0F;
	GPIOA->PUPDR &= 0x50;
	
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
	RCC->CCIPR &= ~(RCC_CCIPR_USART2SEL);
	RCC->CCIPR |= (RCC_CCIPR_USART2SEL_0);
	
	USART_Init(USART2);
	unsigned char read[1] = {0};
	while(1)
	{
		USART_Read(USART2, read, 1);
	}
	
}


