#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>
#include "stm32l476xx.h"
#include "xUART.h"
#include "xTIM.h"

void clkConfig(void);
void gpioConfig(void);
void dacConfig(void); // DAC ch1 PA4
void vLED_Control(void *pvParameters); // led is PA5
void vButton_Control(void *pvParameters); // button is PC13, active low

//bool BTN_ST = false; // global button state
//unsigned int lookup_index = 0; // index for lookup table


QueueHandle_t button_queue;
QueueHandle_t Lookup_index_queue;



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
		bool LED = false; // button state variable
		BaseType_t status;
	//unsigned char read[1] = {0};
    while (true) {
			if(uxQueueMessagesWaiting(button_queue) != 0){
				status = xQueueReceive(button_queue, &LED, 0);
				//status = xQueuePeek(button_queue, &LED, 0);
				if(status == pdPASS){
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
				//xQueueOverwrite(button_queue, &BTN_ST); // update the queue
				xQueueSendToBack(button_queue, &BTN_ST, 1);
				
			}
						
			while(!button){
				button = (GPIOC->IDR&0x2000);
			}
        //vTaskDelay(pdMS_TO_TICKS(10)); // Polling interval
    }
}




int main(void){
	TaskHandle_t ledHandle=NULL;
	TaskHandle_t btnHandle=NULL;
	

  button_queue = 	xQueueCreate(1, sizeof(bool)); // Button State mailbox
  Lookup_index_queue = 	xQueueCreate(1, sizeof(unsigned int)); // lookup index mailbox

	if(button_queue != NULL && Lookup_index_queue != NULL){

		//setup tasks
		if(xTaskCreate(vLED_Control, "LED_Control", 64,NULL, 1, &ledHandle)==pdFAIL){
			// need to set value to stack size
			while(1);
		}
		if(xTaskCreate(vButton_Control, "Button_Control", 64,NULL, 1, &btnHandle)==pdFAIL){
				while(1);
		}
		
		
	//setup hardware
	clkConfig();
	gpioConfig();
	dacConfig();
	uartConfig();
	tim4Config();

	//start task scheduler
	vTaskStartScheduler();

	while(1);
	//return 0;
	}
}