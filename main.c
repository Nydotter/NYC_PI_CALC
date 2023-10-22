/*
 * NYC_PI_CALC.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "ButtonHandler.h"



volatile float LeibnizPi = 0;




extern void vApplicationIdleHook( void );
void vPiLeibniz(void *pvParameters);
void vButtonTask(void *pvParameters);
void controllerTask(void* pvParameters);

TaskHandle_t ledTask;

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
	vInitClock();
	vInitDisplay();
	
	xTaskCreate(controllerTask, (const char *) "control_tsk", configMINIMAL_STACK_SIZE+150, NULL, 3, NULL);
	xTaskCreate(vButtonTask, (const char *) "btTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	//xTaskCreate( vTimeMeasurement, (const char *) "TimeMeasurement", configMINIMAL_STACK_SIZE+10, NULL, 1, &TimeMeasurement);
	xTaskCreate( vPiLeibniz, (const char *) "Leibniz_tsk", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	
	
	vTaskStartScheduler();
	
	return 0;
}

void vPiLeibniz(void* pvParameters)
{
	uint32_t CurIterations = 0;
	float NextSign = 1.0;
		while (1)
		{
			
			LeibnizPi = LeibnizPi + (NextSign / (2 * CurIterations + 1)) * 4;
			NextSign = -NextSign;
			CurIterations++;
		}
}


void vButtonTask(void *pvParameters) {
	initButtons();
	vTaskDelay(3000);
	for(;;) {
		updateButtons();
		vTaskDelay((100/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS);
	}

}

void controllerTask(void* pvParameters) {
	initButtons();
	for(;;) {
		updateButtons();
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			char pistring[12];
			sprintf(&pistring[0], "PI: %.8f", M_PI);
			vDisplayWriteStringAtPos(1,0, "%s", pistring);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			
		}
		if(getButtonPress(BUTTON1) == LONG_PRESSED) {
			
		}
		if(getButtonPress(BUTTON2) == LONG_PRESSED) {
			
		}
		if(getButtonPress(BUTTON3) == LONG_PRESSED) {
			
		}
		if(getButtonPress(BUTTON4) == LONG_PRESSED) {
			
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
}