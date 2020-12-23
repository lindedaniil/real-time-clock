#include "stm32f4xx.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_conf.h"
#include "system_stm32f4xx.h"
#include "misc.h"
#include "SPI.h"

RTC_InitTypeDef rtc;
RTC_TimeTypeDef time;
RTC_TimeTypeDef alarmTime;
RTC_AlarmTypeDef alarm;
EXTI_InitTypeDef exti;
NVIC_InitTypeDef NVIC_InitStructure;

uint8_t ledFlag;

void SysTick_Handler(void)//1ms
{
	CS_ON();
	DMA_Cmd(DMA2_Stream0, ENABLE);
	DMA_Cmd(DMA2_Stream3, ENABLE);
}

void initRTC()
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  RCC_BackupResetCmd(ENABLE);
  RCC_BackupResetCmd(DISABLE);
  RCC_LSICmd(ENABLE);
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
  RCC_RTCCLKCmd(ENABLE);
  RTC_StructInit(&rtc);
  rtc.RTC_HourFormat = RTC_HourFormat_24;
  rtc.RTC_SynchPrediv =  249;
  RTC_Init(&rtc);
}

void RTC_Alarm_IRQHandler()
{
    if(RTC_GetITStatus(RTC_IT_ALRA) != RESET)
    {
      RTC_ClearITPendingBit(RTC_IT_ALRA);
      EXTI_ClearITPendingBit(EXTI_Line17);
			if(ledFlag == 0)
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_15);
				ledFlag = 1;
			}
    }
}

int main(void)
{
	GPIO_InitTypeDef GPIO_struct_Button;
	uint32_t i;
	uint8_t button_pressed = 0;
	ledFlag = 0;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	
	GPIOD->MODER = 0x55000000;
	GPIOD->OTYPER = 0;
	GPIOD->OSPEEDR = 0;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_struct_Button.GPIO_Pin = GPIO_Pin_0;
	GPIO_struct_Button.GPIO_Mode =  GPIO_Mode_IN;
	GPIO_struct_Button.GPIO_Speed = GPIO_Low_Speed ;
	GPIO_struct_Button.GPIO_OType = GPIO_OType_PP;
	GPIO_struct_Button.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOA, &GPIO_struct_Button);
	
	SysTick_Config(SystemCoreClock/1000);//1 ms
	
	
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);

	for(i = 400; i > 0; i--){}
	
	SPI_ini();
	SPI_DMA_ini();
	
	SetSPI_Out(0, 0x8F);//ID
	SetSPI_Out(1, 0x00);
	StartSPI(2);
	SPIwait();
		
	SetSPI_Out(0, 0x20);//Set sample rate
	SetSPI_Out(1, 0x97);//1600
	StartSPI(2);
	SPIwait();
		
	SetSPI_Out(0, 0xA8);//
	SetSPI_Out(1, 0x00);//
	DMA_SetCurrDataCounter(DMA2_Stream0, 7);
	DMA_SetCurrDataCounter(DMA2_Stream3, 7);
	
	__enable_irq();
	initRTC();
	time.RTC_H12 = RTC_HourFormat_24;
        time.RTC_Hours = 12;
        time.RTC_Minutes = 00;
        time.RTC_Seconds = 00;
        RTC_SetTime(RTC_Format_BIN, &time);

	
	EXTI_ClearITPendingBit(EXTI_Line17);
	exti.EXTI_Line = EXTI_Line17;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Rising;
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);
	NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	RTC_AlarmStructInit(&alarm);
	alarmTime.RTC_H12 = RTC_HourFormat_24;
	alarmTime.RTC_Hours = 12;
	alarmTime.RTC_Minutes = 00;
	alarmTime.RTC_Seconds = 15;
	alarm.RTC_AlarmTime = alarmTime;
	alarm.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;
	RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &alarm);
	
	RTC_OutputConfig(RTC_Output_AlarmA, RTC_OutputPolarity_High);
	RTC_ITConfig(RTC_IT_ALRA, ENABLE);
	RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
	RTC_ClearFlag(RTC_FLAG_ALRAF);
	
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
		{
			for(i = 0; i < 10000; i++) {}
			if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1 && button_pressed == 0)
			{   
	      if(ledFlag == 1)
			  {
				  GPIO_ResetBits(GPIOD, GPIO_Pin_15);
				  ledFlag = 0;
			  }
				button_pressed = 1;
				for(i = 0; i < 10000; i++) {}
			}
		}	
		else
		{
			button_pressed = 0;
			for(i = 0; i < 10000; i++) {}
		}
	}
}
