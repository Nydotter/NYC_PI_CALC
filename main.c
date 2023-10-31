/*
 * NYC_PI_CALC.c
 * Created: 20.03.2018 18:32:07
 * Author : Chris
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
#include "stdio.h"
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

volatile double LeibnizPi = 0;
volatile double VietaPi = 0;
volatile double RefPi = 3.1415926;
volatile uint32_t GlobalTimeStart = 0;
volatile uint32_t CurrentTime = 0;

#define ResetBit		( 1 << 0 )
#define LeibResetBit	( 1 << 1 )
#define VietResetBit	( 1 << 2 )
#define TimerRunBit		( 1 << 3 )
#define ProgStateMask	0xFF

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

EventGroupHandle_t ProgState;

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
	
	xTaskCreate( controllerTask, (const char *) "vControl_tsk", configMINIMAL_STACK_SIZE+150, NULL, 3, NULL);
	xTaskCreate( vPiLeibniz, (const char *) "vLeibniz_tsk", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	xTaskCreate( vVietaPi, (const char *) "vVietaPi_tsk", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	xTaskCreate( vCompare, (const char *) "vComp_tsk", configMINIMAL_STACK_SIZE+10, NULL, 2, NULL);
	xTaskCreate( vDisplaytask, (const char *) "vDisp_tsk", configMINIMAL_STACK_SIZE+150, NULL, 3, NULL);
	xTaskCreate( vTimeMeasurement, (const char *) "vTimeMeasurement_tsk", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);
	ProgState = xEventGroupCreate();
	vDisplayClear();
	vTaskStartScheduler();
	
	return 0;
}

/*********************************************************************************
Functions
*********************************************************************************/


void vTimeMeasurement(void* pvParameters){							//Time Function for measuring execution time
TickType_t lasttime = xTaskGetTickCount();
 uint32_t ProgStateVar = 0;
	for(;;) {
		ProgStateVar = (xEventGroupGetBits(ProgState) & ProgStateMask);
		if ( ProgStateVar & TimerRunBit)
		{
			CurrentTime = xTaskGetTickCount();
		}
		if ((ProgStateVar & 0x07) == 0x07) 
		{
			GlobalTimeStart = 0;
			CurrentTime = 0;
			xEventGroupClearBits(ProgState, LeibResetBit);
			xEventGroupClearBits(ProgState, VietResetBit);
			xEventGroupClearBits(ProgState, ResetBit);
			xEventGroupClearBits(ProgState, TimerRunBit);
		}
		vTaskDelayUntil(&lasttime, 10/portTICK_RATE_MS);
	}
}


void vPiLeibniz(void* pvParameters)												//Approximation of Pi by Leibniz Method
{
	uint32_t CurIterations = 0;
	uint32_t ProgStateVar = 0;
	double NextSign = 1.0;
		while (1)
		{
			ProgStateVar = xEventGroupGetBits(ProgState) & ProgStateMask;
			if (State == RunLeibniz)
			{
				LeibnizPi = LeibnizPi + (NextSign / (2.0 * CurIterations + 1)) * 4;
				NextSign = - NextSign;
				CurIterations++;
			}
			if (ProgStateVar & ResetBit)
			{
				LeibnizPi = 0;
				CurIterations = 0;
				NextSign = 1.0;
				xEventGroupSetBits(ProgState, LeibResetBit);
			}
			vTaskDelay(1/portTICK_RATE_MS);
		}
}

void vVietaPi(void* pvParameters)											//Approximation of Pi by Vieta Method
{

	double CurrentApprox = 1;
	double CurrentSqrt = 0;
	uint32_t ProgStateVar = 0;
	
	while(1)
	{
		ProgStateVar = xEventGroupGetBits(ProgState) & ProgStateMask;
		if ( State == RunVieta)
		{
			CurrentSqrt = sqrt(2 + CurrentSqrt); 
			CurrentApprox = CurrentApprox * (CurrentSqrt / 2.0);
			VietaPi = 2 / CurrentApprox;
		}
		if (ProgStateVar & ResetBit)
			{
				CurrentApprox = 1;
				CurrentSqrt = 0;
				VietaPi = 0;
				xEventGroupSetBits(ProgState, VietResetBit);
			}
		vTaskDelay(1/portTICK_RATE_MS);
	}
}


void vCompare(void* pvParameters)														//Comparing Approximated Pi with Reference
{
	uint32_t RoundVietaPi = 0;
	uint32_t RoundLeibPi = 0;
	uint32_t RoundRefPi = 0;
	uint32_t ProgStateVar = 0;
	while(1)
	{
		ProgStateVar = xEventGroupGetBits(ProgState) & ProgStateMask;
		RoundVietaPi = (uint32_t) (VietaPi * 100000);
		RoundLeibPi = (uint32_t) (LeibnizPi * 100000);
		RoundRefPi = (uint32_t) (RefPi * 100000);
		if ((ProgStateVar & TimerRunBit) && (( RoundRefPi == RoundLeibPi) || ( RoundRefPi == RoundVietaPi)))
		{
			xEventGroupClearBits(ProgState, TimerRunBit);
		}
		vTaskDelay(1/portTICK_RATE_MS);
	}
	
}



void vDisplaytask(void* pvParameters)									//Display Task
{
	TickType_t lasttime = xTaskGetTickCount();
	char ApproxPiString[20];
	char RefPiString[20];
	char TitleString[20];
	char TimeString[20];
	int ThouSec = 0;
	int Sec = 0;
	int Min = 0;	
	while(1)
	{
		
		switch(State)
		{
			case RunLeibniz:	
			case StopLeibniz:
				sprintf(&TitleString[0], "Leibniz Approx:");	
				sprintf(&ApproxPiString[0], "ApproxPI: %.7f", LeibnizPi);
				break;
			case RunVieta:
			case StopVieta:
				sprintf(&TitleString[0], "Vieta Approx:  ");
				sprintf(&ApproxPiString[0], "ApproxPI: %.7f", VietaPi);
				break;
			default:
				State = StopLeibniz;
				break;
		}
		ThouSec = (CurrentTime - GlobalTimeStart) % 1000;
		Sec = ((CurrentTime - GlobalTimeStart - ThouSec) % 60000)/1000;
		Min = (((CurrentTime - GlobalTimeStart - ThouSec - Sec) % 3600000)/60000);
		sprintf(&RefPiString[0], "Refer PI: %.7f", RefPi);
		sprintf(&TimeString[0], "Time: %.2i:%.2i:%.3i",Min, Sec, ThouSec);
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
				if (GlobalTimeStart == 0)
				{
					GlobalTimeStart = xTaskGetTickCount();
				}
				xEventGroupSetBits(ProgState, TimerRunBit);
			}
			else if (State == StopVieta)
			{
				State = RunVieta;
				if (GlobalTimeStart == 0)
				{
					GlobalTimeStart = xTaskGetTickCount();
				}
				xEventGroupSetBits(ProgState, TimerRunBit);
			}
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			if (State == RunLeibniz)
			{
				State = StopLeibniz;
				xEventGroupClearBits(ProgState, TimerRunBit);
			}
			else if (State == RunVieta)
			{
				State = StopVieta;	
				xEventGroupClearBits(ProgState, TimerRunBit);
			}
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			xEventGroupSetBits(ProgState, ResetBit);
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
			xEventGroupSetBits(ProgState, ResetBit);
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