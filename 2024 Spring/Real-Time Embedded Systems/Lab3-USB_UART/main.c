#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>
#include "stm32l476xx.h"

void clkConfig(void);
void gpioConfig(void);
void tim4Config(void);
void dacConfig(void); // DAC ch1 PA4
void uartConfig(void); //UART is PA2 and PA3
void vLED_Control(void *pvParameters); // led is PA5
void vButton_Control(void *pvParameters); // button is PC13, active low
void TIM4_IRQHandler(void);
  

//bool BTN_ST = false; // global button state
unsigned int lookup_index = 0; // index for lookup table


QueueHandle_t button_queue;
QueueHandle_t Lookup_index_queue;




const uint16_t sineLookupTable[] = {
0x200, 0x232, 0x264, 0x295, 0x2c4, 0x2f1, 0x31c, 0x345,
0x36a, 0x38c, 0x3aa, 0x3c4, 0x3d9, 0x3ea, 0x3f6, 0x3fe,
0x400, 0x3fe, 0x3f6, 0x3ea, 0x3d9, 0x3c4, 0x3aa, 0x38c,
0x36a, 0x345, 0x31c, 0x2f1, 0x2c4, 0x295, 0x264, 0x232,
0x200, 0x1ce, 0x19c, 0x16b, 0x13c, 0x10f, 0xe4, 0xbb,
0x96, 0x74, 0x56, 0x3c, 0x27, 0x16, 0x0a, 0x02,
0x00, 0x02, 0x0a, 0x16, 0x27, 0x3c, 0x56, 0x74,
0x96, 0xbb, 0xe4, 0x10f, 0x13c, 0x16b, 0x19c, 0x1ce};



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
    TIM4->CR1|=TIM_CR1_ARPE;
	
    TIM4->CR2 &= ~TIM_CR2_MMS;    // Select master mode
    TIM4->CR2 |= TIM_CR2_MMS_2;    // 100 = OC1REF as TRGO
    
    TIM4->DIER |= TIM_DIER_TIE;    // Trigger interrupt enable
    TIM4->DIER |= TIM_DIER_UIE;    // Update interrupt enable
    
    TIM4->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM4->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);  // 0110 = PWM mode 1
    
    TIM4->PSC = 7;            // 16 MHz / (7+1) = 2 MHz timer ticks
    TIM4->ARR = 70;      // 2 MHz / (70+1) = 28.169 kHz interrupt rate; 64 entry look-up table = 440.14 Hz sine wave
    TIM4->CCR1 = 35;        // 50% duty cycle
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1 |= TIM_CR1_CEN;    // Enable timer  
	NVIC_EnableIRQ(TIM4_IRQn); // this makes the interupt actually get called
}
/* based on which note we have, we'll need to change either the PSC value or the ARR value.
	note values:
	a-220Hz , ARR: 141 gives 220.0704
	b-246.94, ARR: 126 gives 246.0630
	c-261.63, ARR: 118 gives 262.605
	d-293.66, ARR: 106 gives 292.0561
	e-329.63, ARR: 94 gives 328.9474
	f-349.23, ARR: 88 gives 351.1236
	g-392, 	  ARR: 79 gives 390.625
	a4-440Hz  ARR: 70 gives 440.14
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

void uartConfig(void){
	//PA2 and PA3
	GPIOA->MODER&=(GPIO_MODER_MODE2|GPIO_MODER_MODE3);
	GPIOA->MODER|=GPIO_MODER_MODE2_1|GPIO_MODER_MODE3_1;
	
	//Change AFR
	GPIOA->AFR[0]|=0x77<<8;
	
	//change output speed
	GPIOA->OSPEEDR|=0xF<<4;
	
	GPIOA->PUPDR &= 0xFFFFFF0F;
	GPIOA->PUPDR &= 0x50;
	
	//change output type
	GPIOA->OTYPER&=~(0x3<<2);
	
	//Enable clock to UART
	RCC->APB1ENR1|=RCC_APB1ENR1_USART2EN;
	
	RCC->CCIPR&=RCC_CCIPR_USART2SEL;
	RCC->CCIPR|=RCC_CCIPR_USART2SEL_0;
	
	//turn on USART
	USART2->CR1&= ~(USART_CR1_UE|USART_CR1_M|USART_CR2_STOP|USART_CR1_PCE|USART_CR1_OVER8);
	
	//baud rate is 9600
	USART2->BRR=1667;
	USART2->CR1|=(USART_CR1_TE|USART_CR1_RE|USART_CR1_UE);
	while((USART2->ISR&USART_ISR_TEACK)==0);
	while((USART2->ISR&USART_ISR_REACK)==0);
}

void USART_Read(USART_TypeDef*USARTx, uint8_t*buffer, int nBytes){
	/*for(uint32_t i=0; i<nBytes; i++){
		while(!(USARTx->ISR&USART_ISR_RXNE));
		buffer[i]=USARTx->RDR;
		
	}
*/

	unsigned char write[5] = {0, 0, 'C', 13, '\n'};
	
	for(int i=0; i < nBytes; i++)
	{
		while(!(USARTx->ISR & USART_ISR_RXNE));
		buffer[i] = USARTx->RDR;
	}
	
	if(buffer[0] == 't' || buffer[0] == 'T')
	{
				GPIOA->ODR |=0x20;
		
		
	}


}



void vLED_Control(void *pvParameters){ // led is PA5
		bool LED; // button state variable
		BaseType_t status;
	//unsigned char read[1] = {0};
    while (true) {
			if(uxQueueMessagesWaiting(button_queue) != 0){
				//status = xQueuePeek(button_queue, &LED, 0);
				xQueuePeek(button_queue, &LED, 0);

				if(LED){
					//turn led on and send sine wave to speaker
					GPIOA->ODR |=0x20;
					DAC1->CR |= DAC_CR_EN1; //enable DAC
				}else{
						//turn led off
					GPIOA->ODR &=0xFFDF;
					DAC1->CR &= ~DAC_CR_EN1; //Disable DAC
				}
			}
		}
}

void vButton_Control(void *pvParameters){ // button is PC13, active low
		bool BTN_ST = false;
		BaseType_t status;
    while (true) {
			bool button = (GPIOC->IDR&0x2000);
			
			if(!button){ // if button is pressed
				if(BTN_ST){ // btn state is true set it to false
					BTN_ST=false;
				}else{
					BTN_ST=true;
				}
				xQueueOverwrite(button_queue, &BTN_ST); // update the queue
			}
			
			
			
	/*
			if(!button && !BTN_ST){ // Poll button state
				BTN_ST=true;
				//status = xQueueSendToBack(button_queue, &BTN_ST, 10); // update the queue
				//status = xQueueOverwrite(button_queue, &BTN_ST); // update the queue
				xQueueOverwrite(button_queue, &BTN_ST); // update the queue
			}
			else if(!button){
				BTN_ST=false;
				//status = xQueueSendToBack(button_queue, &BTN_ST, 10); // update the queue
				//status = xQueueOverwrite(button_queue, &BTN_ST); // update the queue
				xQueueOverwrite(button_queue, &BTN_ST); // update the queue
			}
*/			
			
			
			
			while(!button){
				button = (GPIOC->IDR&0x2000);
			}
        //vTaskDelay(pdMS_TO_TICKS(10)); // Polling interval
    }
}



void TIM4_IRQHandler(void){
	//DAC->DHR12R1 &= 0x000; // reset the DAC value
	//unsigned int lookup_index;
	//BaseType_t status;
	//BaseType_t wake;
	if((TIM4->SR & TIM_SR_CC1IF)!=0){
		/*
		if(uxQueueMessagesWaiting(Lookup_index_queue) != 0){
			status = xQueuePeekFromISR(Lookup_index_queue, &lookup_index); // get the lookup index
		}else{
				lookup_index = 0;
		}
		*/
		if(lookup_index > 63){ // check if the index is out of range
			lookup_index = 0; //reset the index for the lookup table
		}
		
		
		DAC->DHR12R1 &= 0xFFFFF000; // reset the value
		DAC->DHR12R1 |= sineLookupTable[lookup_index]; // Load new value from table
		
		lookup_index ++; // increment index
//		status = xQueueOverwriteFromISR(Lookup_index_queue, &lookup_index, &wake); // overwrite the mailbox (queue of length 1)
	
		TIM4->SR &= ~TIM_SR_CC1IF;
	}
	//check for overflow, generate interrupt
	if((TIM4->SR&TIM_SR_UIF)!=0){
		TIM4->SR &= ~TIM_SR_UIF;	//clear interrupt bit
	}
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
	//uartConfig();
	
	
  button_queue = 	xQueueCreate(1, sizeof(bool)); // Button State mailbox
  Lookup_index_queue = 	xQueueCreate(1, sizeof(unsigned int)); // lookup index mailbox

	if(button_queue != NULL && Lookup_index_queue != NULL){

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
	}
	while(1);
	//return 0;
}
