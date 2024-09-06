#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include "stm32l476xx.h"

void clkConfig(void);
void gpioConfig(void);
void tim4Config(void);
void dacConfig(void); // DAC ch1 PA4
void vLED_Control(void *pvParameters); // led is PA5
void vButton_Control(void *pvParameters); // button is PC13, active low


void TIM4_IRQHandler(void);
  

bool BTN_ST = false; // global button state
unsigned int lookup_index = 0; // index for lookup table

const uint16_t sineLookupTable[] = {
0x200, 0x232, 0x264, 0x295, 0x2c4, 0x2f1, 0x31c, 0x345,
0x36a, 0x38c, 0x3aa, 0x3c4, 0x3d9, 0x3ea, 0x3f6, 0x3fe,
0x400, 0x3fe, 0x3f6, 0x3ea, 0x3d9, 0x3c4, 0x3aa, 0x38c,
0x36a, 0x345, 0x31c, 0x2f1, 0x2c4, 0x295, 0x264, 0x232,
0x200, 0x1ce, 0x19c, 0x16b, 0x13c, 0x10f, 0xe4, 0xbb,
0x96, 0x74, 0x56, 0x3c, 0x27, 0x16, 0x0a, 0x02,
0x00, 0x02, 0x0a, 0x16, 0x27, 0x3c, 0x56, 0x74,
0x96, 0xbb, 0xe4, 0x10f, 0x13c, 0x16b, 0x19c, 0x1ce};

/*
const uint16_t sineLookupTable[] = {
2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
2048, 1847, 1648, 1453, 1264, 1082, 910, 749,
600, 465, 345, 242, 156, 88, 39, 10,
0, 10, 39, 88, 156, 242, 345, 465,
600, 749, 910, 1082, 1264, 1453, 1648, 1847};
*/
// this is table in hex for debugging purposes
/*
const uint16_t sineLookupTable[] = {
0x800, 0x8c8, 0x98f, 0xa52, 0xb0f, 0xbc5, 0xc71, 0xd12,
0xda7, 0xe2e, 0xea6, 0xf0d, 0xf63, 0xfa7, 0xfd8, 0xff5,
0xfff, 0xff5, 0xfd8, 0xfa7, 0xf63, 0xf0d, 0xea6, 0xe2e,
0xda7, 0xd12, 0xc71, 0xbc5, 0xb0f, 0xa52, 0x98f, 0x8c8,
0x800, 0x737, 0x670, 0x5ad, 0x4f0, 0x43a, 0x38e, 0x2ed,
0x258, 0x1d1, 0x159, 0xf2, 0x9c, 0x58, 0x27, 0x0a,
0x00, 0x0a, 0x27, 0x58, 0x9c, 0xf2, 0x159, 0x1d1,
0x258, 0x2ed, 0x38e, 0x43a, 0x4f0, 0x5ad, 0x670, 0x737};
*/

void clkConfig(void){
	RCC->CR|=RCC_CR_HSION;
	while((RCC->CR&RCC_CR_HSIRDY)==0);
	RCC->CFGR|=RCC_CFGR_SW_HSI;
	SystemCoreClockUpdate(); //updates FreeRTOS clock
}


void gpioConfig(void){
		//output for LED
	RCC->AHB2ENR|=RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER&=~GPIO_MODER_MODE5;//led
	GPIOA->MODER|=GPIO_MODER_MODE5_0;
		//input for BTN
	RCC->AHB2ENR|=RCC_AHB2ENR_GPIOCEN;
	GPIOC->MODER&=~GPIO_MODER_MODE13;
}



void tim4Config(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;        // Enable TIM4 clock
    TIM4->CR1&=~TIM_CR1_CEN;
    TIM4->CR1 &= ~TIM_CR1_CMS;    // Edge-aligned mode
    TIM4->CR1 &= ~TIM_CR1_DIR;    // Up-counting
    
    TIM4->CR2 &= ~TIM_CR2_MMS;    // Select master mode
    TIM4->CR2 |= TIM_CR2_MMS_2;    // 100 = OC1REF as TRGO
    
    TIM4->DIER |= TIM_DIER_TIE;    // Trigger interrupt enable
    TIM4->DIER |= TIM_DIER_UIE;    // Update interrupt enable
    
    TIM4->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM4->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);  // 0110 = PWM mode 1
    
    TIM4->PSC = 7;            // 16 MHz / (7+1) = 2 MHz timer ticks
    TIM4->ARR = 70;        // 2 MHz / (70+1) = 28.169 kHz interrupt rate; 64 entry look-up table = 440.14 Hz sine wave
    TIM4->CCR1 = 35;        // 50% duty cycle
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1 |= TIM_CR1_CEN;    // Enable timer  
		NVIC_EnableIRQ(TIM4_IRQn); // this makes the interupt actually get called
}





/*
void tim4Config(void){
	//enable clock
	RCC->APB1ENR1|=RCC_APB1ENR1_TIM4EN;
	//configure control registers
	TIM4->CR1&=~TIM_CR1_DIR; //controls direction of timer (up/down)
	TIM4->CR1&=~TIM_CR1_CMS;
	TIM4->CR2&=TIM_CR2_MMS;
	TIM4->CR2|=TIM_CR2_MMS_2;
	
	//configure dma/interrupt control
	TIM4->DIER|=TIM_DIER_TIE;
	TIM4->DIER|=TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM4_IRQn);
	//NVIC_SetPriority(TIM4_IRQn,0);
	//output compare mode
	
	//configure prescalar   
	//440*64=28160
	// divide by 8
	TIM4->PSC=7; 
	//configure ARR
	TIM4->ARR=770;
	//set duty cycle
	TIM4->CCR1=30; //or 279? 35usec to update table
	TIM4->CCMR1&=~TIM_CCMR1_OC1M;
	TIM4->CCMR1|=(
	TIM4->CCMR1|=TIM_CCMR_CC1S_EN;
	TIM4->CCER|=~TIM_CCER_CC1P_EN;
	TIM4->CCER|=TIM_CCER_CC1E_EN;
	//set CCxIE and/or CCxDE bits for interrupt
	//URS 1?
	
	TIM4->MMS|=100;
	//configure as output

	//TIM4->CCER|= ;
	//TIM4->BDTR|= TIM_BDTR_MOE //main output enable 
	//enable timer
	TIM4->CR1|=TIM_CR1_CEN;
}
*/

void dacConfig(void){ //double check this
	RCC->APB1ENR1|=RCC_APB1ENR1_DAC1EN;
	//disable to change settings
	DAC->CR&=~(DAC_CR_EN1); // Disable DAC

	GPIOA->MODER &= ~(GPIO_MODER_MODE4);//dac
	GPIOA->MODER |= 0x00000300;
	
	DAC->CR |= DAC_CR_TEN1;
	DAC->CR &= ~(DAC_CR_TSEL1);
	DAC->CR |= DAC_CR_TSEL1_0;
	DAC->CR |= DAC_CR_TSEL1_2;
	
	// DAC->MCR &=0xFFFFFFF8 // may need to try this instead of the nex three lines ======
	DAC1->MCR &= ~(DAC_MCR_MODE1_0); 
	DAC1->MCR |= DAC_MCR_MODE1_1;
	DAC1->MCR &= ~(DAC_MCR_MODE1_2);
	//=================================================================================
	
	DAC->CR |= DAC_CR_EN1; //enable
	
}

void vLED_Control(void *pvParameters){ // led is PA5
    while (true) {
			if(BTN_ST){
				//turn led on and send sine wave to speaker
				GPIOA->ODR |=0x20;
				//DAC_DHR12R1|= ;
				DAC1->CR |= DAC_CR_EN1; //enable
			}else{
				//turn led off
				GPIOA->ODR &=0xFFDF;
				DAC1->CR &= ~DAC_CR_EN1; //enable
			}
    }
}

void vButton_Control(void *pvParameters){ // button is PC13, active low
	
    while (true) {
			bool button = (GPIOC->IDR&0x2000);
			
			if(!button && !BTN_ST){ // Poll button state
				BTN_ST=true;	
			}
			else if(!button){
				BTN_ST=false;	
			}
      while(!button){
				button = (GPIOC->IDR&0x2000);
			}

        //vTaskDelay(pdMS_TO_TICKS(10)); // Polling interval
    }
}



void TIM4_IRQHandler(void){
	//DAC->DHR12R1 &= 0x000; // reset the DAC value
	if((TIM4->SR & TIM_SR_CC1IF)!=0){
		DAC->DHR12R1 &= 0xFFFFF000; // reset the value
		DAC->DHR12R1 |= sineLookupTable[lookup_index]; // Load new value from table
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
	
	
	//clear interrupt bit
}
int main(void){
	TaskHandle_t ledHandle=NULL;
	TaskHandle_t btnHandle=NULL;
	//TaskHandle_t sinHandle=NULL;
	
	//setup hardware
	clkConfig();
	gpioConfig();
	tim4Config();
	dacConfig();
	
	
	//setup tasks
	if(xTaskCreate(vLED_Control, "LED_Control", 10,NULL, 1, &ledHandle)==pdFAIL){
		// need to set value to stack size
		while(1);
	}
	if(xTaskCreate(vButton_Control, "Button_Control", 10,NULL, 1, &btnHandle)==pdFAIL){
			while(1);
	}

	//start task scheduler
	vTaskStartScheduler();

	while(1);
	//return 0;
}
