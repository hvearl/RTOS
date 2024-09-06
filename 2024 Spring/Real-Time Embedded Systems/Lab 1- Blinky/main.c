#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include "stm32l476xx.h"


bool BTN_ST = false; // global button state


void vLED_Control(void *pvParameters){ // led is PA5
    while (true) {
			if(BTN_ST){
				//turn led on
				GPIOA->ODR |=0x20;
			}else{
				//turn led off
				GPIOA->ODR &=0xffdf;
			}
        // Wait for button press signal
        //ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Toggle LED state
        //gpio_write(LED_PIN, !gpio_read(LED_PIN));
			  //vTaskDelay(pdMS_TO_TICKS(100)); // Polling interval

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
int main(void){
	BaseType_t xLED_return;
	BaseType_t xBTN_return;
	
	//setup hardware
	//output for LED
	RCC->AHB2ENR|=RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER&=~GPIO_MODER_MODE5;
	GPIOA->MODER|=GPIO_MODER_MODE5_0;
	
	//input for BTN
	RCC->AHB2ENR|=RCC_AHB2ENR_GPIOCEN;
	GPIOC->MODER&=~GPIO_MODER_MODE13;
	
	
	//setup tasks
	xLED_return=xTaskCreate(vLED_Control, "LED_Control", 10,NULL, 1, NULL); // need to set value to stack size
	if(xLED_return==pdPASS){
		xBTN_return=xTaskCreate(vButton_Control, "Button_Control", 10,NULL, 1, NULL);
		if(xBTN_return==pdPASS){
			//start task scheduler
			vTaskStartScheduler();
		}
	}
	

	for(;;);
	return 0;
}