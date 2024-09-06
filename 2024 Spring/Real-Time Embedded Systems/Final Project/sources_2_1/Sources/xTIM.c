#include "xTIM.h"
#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "queue.h"



unsigned int lookup_index=0;
unsigned int vol = 0;
extern QueueHandle_t dac_Volume;


const uint16_t sineLookupTable[] = {
0x200, 0x232, 0x264, 0x295, 0x2c4, 0x2f1, 0x31c, 0x345,
0x36a, 0x38c, 0x3aa, 0x3c4, 0x3d9, 0x3ea, 0x3f6, 0x3fe,
0x400, 0x3fe, 0x3f6, 0x3ea, 0x3d9, 0x3c4, 0x3aa, 0x38c,
0x36a, 0x345, 0x31c, 0x2f1, 0x2c4, 0x295, 0x264, 0x232,
0x200, 0x1ce, 0x19c, 0x16b, 0x13c, 0x10f, 0xe4, 0xbb,
0x96, 0x74, 0x56, 0x3c, 0x27, 0x16, 0x0a, 0x02,
0x00, 0x02, 0x0a, 0x16, 0x27, 0x3c, 0x56, 0x74,
0x96, 0xbb, 0xe4, 0x10f, 0x13c, 0x16b, 0x19c, 0x1ce};


void tim4Config(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;        // Enable TIM4 clock
    TIM4->CR1&=~TIM_CR1_CEN;
    TIM4->CR1 &= ~TIM_CR1_CMS;    // Edge-aligned mode
    TIM4->CR1 &= ~TIM_CR1_DIR;	// Up-counting
    TIM4->CR1 |=TIM_CR1_ARPE;
	
    TIM4->CR2 &= ~TIM_CR2_MMS;    // Select master mode
    TIM4->CR2 |= TIM_CR2_MMS_2;    // 100 = OC1REF as TRGO
    
    TIM4->DIER |= TIM_DIER_TIE;    // Trigger interrupt enable
    TIM4->DIER |= TIM_DIER_UIE;    // Update interrupt enable
    
    TIM4->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM4->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);  // 0110 = PWM mode 1
    
    TIM4->PSC = 3;            // 16 MHz / (3+1) = 4 MHz timer ticks
    TIM4->ARR = 283;      // 4 MHz / (70+1) = _ kHz interrupt rate; 64 entry look-up table = 440.14 Hz sine wave
    TIM4->CCR1 = 70;        // 50% duty cycle
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1 |= TIM_CR1_CEN;    // Enable timer  
	NVIC_EnableIRQ(TIM4_IRQn); // this makes the interupt actually get called
}


void TIM4_IRQHandler(void){
	//DAC->DHR12R1 &= 0x000; // reset the DAC value
	if((TIM4->SR & TIM_SR_CC1IF)!=0){
		DAC->DHR12R1 &= 0xFFFFF000; // reset the value
		
		
		if(uxQueueMessagesWaitingFromISR(dac_Volume) != 0){
			xQueueReceiveFromISR(dac_Volume, &vol, pdFALSE);
		}
		
		if(vol<= 2){
			vol = 0;
		}
		
		DAC->DHR12R1 |= ((sineLookupTable[lookup_index]/8)*vol); // Load new value from table
		lookup_index ++; // increment index
	
		if(lookup_index > 63){ // check if the index is out of range
			lookup_index = 0; //reset the index for the lookup table
		}
		TIM4->SR &= ~TIM_SR_CC1IF;
	}
	//check for overflow, generate interrupt
	if((TIM4->SR&TIM_SR_UIF)!=0){
		TIM4->SR &= ~TIM_SR_UIF;	//clear interrupt bit
	}
}
