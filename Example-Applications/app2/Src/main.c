/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Muhammed Cemal Eryigit
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include "stm32h7xx.h"


void USART_Config(uint32_t baud);
void GPIO_Config();
void SendTxt(char *Adr);
void printChar(char c);


int main(void)
{
	GPIO_Config();
	USART_Config(9600);

	SendTxt("Here is app 2\n");
	 while(1)
	    {

	    }

	 return 0;
}


void USART_Config(uint32_t baud) //Bitti
{
	RCC->APB1LENR |= 0x40000;

	USART3->BRR = (uint16_t)(SystemCoreClock / baud);		// BaudRate
	USART3->CR1 |= 1 << 2;		// Rx enable PD9
	USART3->CR1 |= 1 << 3;		// Tx enable //PD8
	USART3->CR1 |= 0 << 10;		// Parity control disable
	USART3->CR1 |= 0 << 12;		// Word length 8bit
	USART3->CR2 |= 0 << 12;		// Stop bit 1
	USART3->CR1 |= 1 << 0;		// Usart enable

	NVIC_EnableIRQ(USART3_IRQn);
	NVIC_SetPriority(USART3_IRQn , 1);

}
void GPIO_Config() //Usart için config edildi //Bitti
{

	RCC->AHB4ENR  |= 0x8;		// GPIOD Clock Enable
	GPIOD->MODER  &= 0xAFFFF;		// AF PD8 AND PD9
	GPIOD->AFR[1] |= (7 << 0) | (7 << 4);		// PD8 & PD9 AF7 (USART3)
}
void printChar(char c){

	while(!(USART3->ISR & 0x80)); // 8.bit transmission complete
	USART3->TDR = (uint8_t) c;
}
void SendTxt(char *Adr)
{
	while(*Adr)
	{
		printChar(*Adr);
		Adr++;
	}
}
