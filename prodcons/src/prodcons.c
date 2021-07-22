#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "stdbool.h"
#include "driverbuttons.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#define BUFFER_SIZE 32

osThreadId_t consumidor_id;
osSemaphoreId_t cheio_id;
uint8_t buffer[BUFFER_SIZE];

int tempo_inicial = 0;

void GPIOJ_Handler(void)// ISR produtora
{
  int tempo_kernel = osKernelGetTickCount();
  int anti_bouncing = tempo_kernel - tempo_inicial;
  if(anti_bouncing >= 200)
  {
    osSemaphoreRelease(cheio_id); // há espaço disponível?
    tempo_inicial = tempo_kernel;
  }
  GPIOIntClear(GPIO_PORTJ_BASE, GPIO_INT_PIN_0);
} // GPIOJ_Handler


void consumidor(void *arg)
{
  uint8_t i = 0;
  
  while(1)
  {
    osSemaphoreAcquire(cheio_id, osWaitForever); // há dado disponível?
    i++;// incrementa o índice de retirada do buffer
    if(i >= BUFFER_SIZE) // reseta o buffer caso tenha chegado no limite
    {
      i = 0;
    }
    LEDWrite(LED4 | LED3 | LED2 | LED1, i & 0xF); // acende os leds consumindo a informação
  } // while
} // consumidor


void main(void)
{
  SystemInit();
  
  LEDInit(LED4 | LED3 | LED2 | LED1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); 
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);
  GPIOPadConfigSet(GPIO_PORTJ_BASE ,GPIO_PIN_0,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
  GPIOIntDisable(GPIO_PORTJ_BASE, GPIO_INT_PIN_0);
  GPIOIntTypeSet(GPIO_PORTJ_BASE,GPIO_PIN_0,GPIO_FALLING_EDGE);
  GPIOIntRegister(GPIO_PORTJ_BASE, GPIOJ_Handler);
  GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_INT_PIN_0);
  
  osKernelInitialize();

  consumidor_id = osThreadNew(consumidor, NULL, NULL);
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0
  
  if(osKernelGetState() == osKernelReady)
  {
    osKernelStart();
  }

  while(1);
} // main

