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
void vLED_Control(void *pvParameters); 		// led is PA5
void vButton_Control(void *pvParameters); // button is PC13, active low
void vUART_Control(void *pvParameters); 	// PB7 is RX, PB6 is TX
void vGate_Keeper(void *pvParameters);		// UART2	tx control

//bool BTN_ST = false; // global button state
//unsigned int lookup_index = 0; // index for lookup table


QueueHandle_t button_queue;
QueueHandle_t Lookup_index_queue;
QueueHandle_t uart_mailbox;
QueueHandle_t uart_Volume;
QueueHandle_t dac_Volume;


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
	DAC->CR&=~(DAC_CR_EN1); // Disable DAC
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

void vUART_Control(void *pvParameters){
	//unsigned int buffer;
	//char RX[20];
	
	uint16_t buffer_1 = 0;
	uint16_t buffer_2 = 0;
	uint16_t prev = 0;
	uint16_t prev_note = 0;
	uint8_t switch_count = 0;
	
	while(true){
		if(switch_count == 0){
			vTaskSuspendAll(); // suspend scheduler so we send uart
			
			if(USART3->ISR&USART_ISR_TXE){
						USART3->TDR=0x55&0xFF;
			}
			// PC11 is RX, PC10 is TX
			buffer_1 = 0;
			buffer_2 = 0;
			while(!(USART3->ISR & USART_ISR_RXNE)); //check event
				buffer_1=USART3->RDR;
					
			while(!(USART3->ISR & USART_ISR_RXNE)); //check event
				buffer_2=USART3->RDR;
				buffer_2 |= buffer_1 << (8);						
				buffer_2 = buffer_2 / 25; // divide by 25.4 to convert to inches
				
				
			xTaskResumeAll(); // resume scheduler so we can adjust other tasks like gatekeeper


			if(prev != buffer_2 && buffer_2 < 8){
				prev = buffer_2;
				xQueueSendToBack(uart_Volume, &buffer_2, 100);
				xQueueSendToBack(dac_Volume, &prev, 0);
			}
						

		}else if(switch_count == 1){
			// request from other sensor using different uart
			vTaskSuspendAll(); // suspend scheduler so we send uart
			
			if(USART1->ISR&USART_ISR_TXE){
						USART1->TDR=0x55&0xFF;
			}
			// PB7 is RX, PB6 is TX
			buffer_1 = 0;
			buffer_2 = 0;
			while(!(USART1->ISR & USART_ISR_RXNE)); //check event
				buffer_1=USART1->RDR;
					
			while(!(USART1->ISR & USART_ISR_RXNE)); //check event
				buffer_2=USART1->RDR;
				buffer_2 |= buffer_1 << (8);						
				buffer_2 = buffer_2 / 10; // divide by 10 to get all the notes in the sensor range
				
				
			xTaskResumeAll(); // resume scheduler so we can adjust other tasks like gatekeeper


			if(prev_note != buffer_2 && buffer_2 <= 21){
				prev_note = buffer_2;
				xQueueSendToBack(uart_mailbox, &buffer_2, 1000);
			
			switch(buffer_2){
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
				TIM4->ARR=283;
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
		
		
		for(int k = 0; k<20000; k++); // need this delay for sensor
		switch_count++;
		if(switch_count > 1){
			switch_count = 0; // reset switch_count
		}
	}
}



void vGate_Keeper(void *pvParameters){
	unsigned int buffer; // note distance
	unsigned int volume; // volume distance
	
	char RX[20];
	while(true){
		
		if(xQueueReceive(uart_mailbox,&buffer,50)==pdTRUE){
			int len = sprintf(RX, "Note: %d\n", buffer);
							
							
			for(int i = 0; i< len; ){
				if(USART2->ISR&USART_ISR_TXE){
						USART2->TDR=RX[i];
						RX[i] = NULL;
						i++;
				}
			}
		}
		
		if(xQueueReceive(uart_Volume,&volume,50)==pdTRUE){
			int len = sprintf(RX, "Volume: %d\n", volume);
									
			for(int i = 0; i< len; ){
				if(USART2->ISR&USART_ISR_TXE){
						USART2->TDR=RX[i];
						RX[i] = NULL;
						i++;
				}
			}
		}
	
		
	}
}


int main(void){
	TaskHandle_t ledHandle=NULL;
	TaskHandle_t btnHandle=NULL;
	TaskHandle_t UARTHandle=NULL;
	TaskHandle_t GateKeeper=NULL;
	

  button_queue = 	xQueueCreate(1, sizeof(bool)); // Button State mailbox
  Lookup_index_queue = 	xQueueCreate(1, sizeof(unsigned int)); // lookup index mailbox
	uart_mailbox = 	xQueueCreate(4, sizeof(unsigned int));
	uart_Volume = xQueueCreate(4, sizeof(unsigned int));
	dac_Volume = xQueueCreate(4, sizeof(unsigned int));

	
	if(button_queue != NULL && Lookup_index_queue != NULL && uart_mailbox!=NULL && uart_Volume != NULL && dac_Volume !=NULL){

		//setup tasks
		if(xTaskCreate(vLED_Control, "LED_Control", configMINIMAL_STACK_SIZE,NULL, 1, &ledHandle)==pdFAIL){
			// need to set value to stack size
			while(1);
		}
		if(xTaskCreate(vButton_Control, "Button_Control", configMINIMAL_STACK_SIZE,NULL, 1, &btnHandle)==pdFAIL){
				while(1);
		}
		if(xTaskCreate(vUART_Control, "UART_Control", configMINIMAL_STACK_SIZE,NULL, 1, &UARTHandle)==pdFAIL){
				while(1);
		}
		if(xTaskCreate(vGate_Keeper, "GAte_Keeper", configMINIMAL_STACK_SIZE,NULL, 1, &GateKeeper)==pdFAIL){
				while(1);
		}	
		
		
		
		
	//setup hardware
	clkConfig();
	gpioConfig();
	dacConfig();
	uart2Config();
	uart1Config();
	uart3Config();
	tim4Config();

	//start task scheduler
	vTaskStartScheduler();

	while(1);
	//return 0;
	}
}
