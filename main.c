/*
 * NYC_PI_CALC.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : Cwis
 */ 


/*********************************************************************************
Includes
*********************************************************************************/

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"
#include "math.h"

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


/*********************************************************************************
Global variables
*********************************************************************************/

volatile float LeibnizPi = 0;
volatile double VietaPi = 0;
volatile float RefPi = 3.14159265359;
volatile int GlobalSec = 0;
volatile int GlobalMin = 0;
volatile int GlobalHunSec = 0;
volatile bool Reset = 0;
volatile bool TimerRuning = 0;

typedef enum {StopLeibniz, StopVieta, RunLeibniz, RunVieta}StateType;
volatile StateType State = StopLeibniz;


/*********************************************************************************
Function Definitions
*********************************************************************************/

extern void vApplicationIdleHook( void );
void vPiLeibniz(void *pvParameters);
void controllerTask(void* pvParameters);
void vVietaPi(void* pvParameters);
void vCompare(void* pvParameters);
void vDisplaytask(void* pvParameters);
void vTimeMeasurement(void* pvParameters);

/*********************************************************************************
Idle Task
*********************************************************************************/

void vApplicationIdleHook( void )
{	
	
}

/*********************************************************************************
Main
*********************************************************************************/

int main(void)
{
	vInitClock();
	vInitDisplay();
	
	xTaskCreate(controllerTask, (const char *) "vControl_tsk", configMINIMAL_STACK_SIZE+150, NULL, 3, NULL);
	xTaskCreate( vPiLeibniz, (const char *) "vLeibniz_tsk", configMINIMAL_STACK_SIZE+10, NULL, 2, NULL);
	xTaskCreate( vVietaPi, (const char *) "vVietaPi_tsk", configMINIMAL_STACK_SIZE+10, NULL, 2, NULL);
	xTaskCreate( vCompare, (const char *) "vComp_tsk", configMINIMAL_STACK_SIZE+10, NULL, 3, NULL);
	xTaskCreate( vDisplaytask, (const char *) "vDisp_tsk", configMINIMAL_STACK_SIZE+150, NULL, 2, NULL);
	xTaskCreate( vTimeMeasurement, (const char *) "vTimeMeasurement_tsk", configMINIMAL_STACK_SIZE+100, NULL, 3, NULL);
	
	vTaskStartScheduler();
	
	return 0;
}

/*********************************************************************************
Functions
*********************************************************************************/


void vTimeMeasurement(void* pvParameters){							//Time Function for measuring execution time
TickType_t lasttime = xTaskGetTickCount();
	for(;;) {
		if (TimerRuning)
		{
			
			GlobalHunSec++;
			if(GlobalHunSec >= 100) {
				GlobalHunSec = 0;
				GlobalSec++;
			}
			if (GlobalSec >= 60){
				GlobalSec = 0;
				GlobalMin++;
			}
			if(GlobalMin >= 60) {
				GlobalMin = 0;
			}
		}
		if (Reset)
		{
			GlobalSec = 0;
			GlobalMin = 0;
			Reset = 0;
			TimerRuning = 0;
		}
		vTaskDelayUntil(&lasttime, 10/portTICK_RATE_MS);
	}
}


void vPiLeibniz(void* pvParameters)												//Approximation of Pi by Leibniz Method
{
	uint32_t CurIterations = 0;
	float NextSign = 1.0;
		while (1)
		{
			if (State == RunLeibniz)
			{
				LeibnizPi = LeibnizPi + (NextSign / (2 * CurIterations + 1)) * 4;
				NextSign = - NextSign;
				CurIterations++;
			}
			if (Reset)
			{
				LeibnizPi = 0;
				CurIterations = 0;
				NextSign = 1.0;
			}
			vTaskDelay(5/portTICK_RATE_MS);
		}
}

void vVietaPi(void* pvParameters)											//Approximation of Pi by Vieta Method
{

	double CurrentApprox = 1;
	double CurrentSqrt = 0;
	
	while(1)
	{
		
		if ( State == RunVieta)
		{
			CurrentSqrt = sqrt(2 + CurrentSqrt); 
			CurrentApprox = CurrentApprox * (CurrentSqrt / 2.0);
			VietaPi = 2 / CurrentApprox;
		}
		if (Reset)
			{
				CurrentApprox = 1;
				CurrentSqrt = 0;
				VietaPi = 0;
			}
		vTaskDelay(5/portTICK_RATE_MS);
	}
}


void vCompare(void* pvParameters)														//Comparing Approximated Pi with Reference
{
	uint32_t RoundVietaPi = 0;
	uint32_t RoundLeibPi = 0;
	uint32_t RoundRefPi = 0;
	while(1)
	{
		RoundVietaPi = (uint32_t) (VietaPi * 10e4);
		RoundLeibPi = (uint32_t) (LeibnizPi * 10e4);
		RoundRefPi = (uint32_t) (RefPi * 10e4);
		if (TimerRuning && (( RoundRefPi == RoundLeibPi) || ( RoundRefPi == RoundVietaPi)))
		{
			TimerRuning = 0;
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
	
}



void vDisplaytask(void* pvParameters)									//Display Task
{
	TickType_t lasttime = xTaskGetTickCount();

	char ApproxPiString[20];
	char RefPiString[20];
	char TitleString[20];
	char TimeString[20];
	
	while(1)
	{
		switch(State)
		{
			
		case RunLeibniz:	
		case StopLeibniz:
			sprintf(&TitleString[0], "Leibniz Approx:");	
			sprintf(&ApproxPiString[0], "ApproxPI: %.8f", LeibnizPi);
			break;
		case RunVieta:
		case StopVieta:
			sprintf(&TitleString[0], "Vieta Approx:  ");
			sprintf(&ApproxPiString[0], "ApproxPI: %.8f", VietaPi);
			break;
		default:
			State = StopLeibniz;
			break;
		}
	sprintf(&RefPiString[0], "Refer PI: %.8f", RefPi);
	sprintf(&TimeString[0], "Time: %.2i:%.2i:%.2i", GlobalMin, GlobalSec, GlobalHunSec);
	vDisplayWriteStringAtPos(0,0, "%s", TitleString);	
	vDisplayWriteStringAtPos(1,0, "%s", ApproxPiString);	
	vDisplayWriteStringAtPos(2,0, "%s", RefPiString);	
	vDisplayWriteStringAtPos(3,0, "%s", TimeString);
	vTaskDelayUntil(&lasttime, 500/portTICK_RATE_MS);
	}

}


void controllerTask(void* pvParameters) {
	initButtons();
	for(;;) {
		updateButtons();
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			if (State == StopLeibniz)
			{
				State = RunLeibniz;
				TimerRuning = 1;
			}
			else if (State == StopVieta)
			{
				State = RunVieta;
				TimerRuning = 1;
			}
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			if (State == RunLeibniz)
			{
				State = StopLeibniz;
				TimerRuning = 0;
			}
			else if (State == RunVieta)
			{
				State = StopVieta;	
				TimerRuning = 0;
			}
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			Reset = 1;
			if (State == RunLeibniz)
			{
				State = StopLeibniz;
			}
			else if (State == RunVieta)
			{
				State = StopVieta;	
			}
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			if(State == StopLeibniz)
			{
				State = StopVieta;
			}
			else if (State == StopVieta)
			{
				State = StopLeibniz;
			}
			else if(State == RunLeibniz)
			{
				State = RunVieta;
			}
			else if (State == RunVieta)
			{
				State = RunLeibniz;
			}	
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
}