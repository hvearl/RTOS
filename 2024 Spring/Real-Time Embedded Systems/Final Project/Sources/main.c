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
	unsigned int buffer;
	char RX[20];
	while(true){
			//distance = 'B';
			if(xQueueReceive(uart_mailbox,&buffer,0)==pdTRUE){
						/*
							if(USART2->ISR&USART_ISR_TXE){
									USART2->TDR=buffer&0xFF;
							}	else{
								USART2->CR1&=~USART_CR1_TXEIE;
							}
						*/
				switch(buffer){
					case 'p':
						if(USART1->ISR&USART_ISR_TXE){
									USART1->TDR=0x55&0xFF;
						}
						// PB7 is RX, PB6 is TX
						uint16_t buffer_1 = 0;
						uint16_t buffer_2 = 0;
						while(!(USART1->ISR & USART_ISR_RXNE)); //check event
							buffer_1=USART1->RDR;
						
						while(!(USART1->ISR & USART_ISR_RXNE)); //check event
							buffer_2=USART1->RDR;
							buffer_2 |= buffer_1 << (8);
						
							
							buffer_2 = buffer_2 / 25; // divide by 25.4 to convert to inches
							xQueueSendToBack(uart_Volume, &buffer_2, 1);
							/*
							int len = sprintf(RX, "%d in.  ", buffer_2);
							
							
							for(int i = 0; i< len; ){
							if(USART2->ISR&USART_ISR_TXE){
									USART2->TDR=RX[i];
									RX[i] = NULL;
									i++;
							}
						}
						*/
						break;
				}
			} //end mailbox

	}
}



void vGate_Keeper(void *pvParameters){
	unsigned int buffer; // note distance
	unsigned int volume; // volume distance
	char RX[20];
	while(true){
		if(xQueueReceive(uart_mailbox,&buffer,0)==pdTRUE){
			int len = sprintf(RX, "Note: %d", buffer);
							
							
			for(int i = 0; i< len; ){
				if(USART2->ISR&USART_ISR_TXE){
						USART2->TDR=RX[i];
						RX[i] = NULL;
						i++;
				}
						}
		}
		if(xQueueReceive(uart_Volume,&volume,0)==pdTRUE){
			int len = sprintf(RX, "Volume: %d", volume);
									
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
	uart_mailbox = 	xQueueCreate(1, sizeof(unsigned int));
	uart_Volume = xQueueCreate(1, sizeof(unsigned int));
	
	if(button_queue != NULL && Lookup_index_queue != NULL && uart_mailbox!=NULL && uart_Volume != NULL){

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
	//uart4Config();
	uart2Config();
	uart1Config();
	tim4Config();
	tim3Config();

	//start task scheduler
	vTaskStartScheduler();

	while(1);
	//return 0;
	}
}