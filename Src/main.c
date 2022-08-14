/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Muhammed Cemal Eryigit
 ******************************************************************************
 */
#include "stm32h7xx.h"
//SystemCoreClock = 64 MHz

//Functions
void USART_Config(uint32_t baud);
void GPIO_Config();
void SendTxt(char *Adr);
void printChar(char c);
void delay(volatile uint32_t second);
void FLASH_UnLocker(uint8_t bank);
void FLASH_Locker(uint8_t bank);
void FLASH_Erase(uint8_t SER, uint8_t SSB);
void FLASH_Write(uint32_t address, uint32_t data);
static void goto_application();
void SysTick_init(uint32_t number);
uint8_t BootloaderIsTrigger();
void Bootloader_menu(uint8_t BootloaderIsTrigger);
void Delete_App_1();
void Write_App_1(uint32_t *data);
void Write_App_2(uint32_t *data);
void Delete_App_2();
void downloaded();


//constant variables
#define app1_add_start 0x08040000
#define app1_add_end 0x08080000
#define app2_add_start 0x08120000
#define app2_add_end 0x08160000

//variables
volatile uint32_t down_k=0;
volatile uint8_t down_i=0;
volatile uint32_t say =0;
volatile uint8_t ok =0;
volatile uint8_t Write_App_1_check=0;
volatile uint8_t Write_App_2_check=0;
volatile uint32_t delay_count = 0;
volatile uint32_t download_count = 0;
volatile uint32_t flash_count = 0;
volatile uint8_t whichApp ='\0';
volatile uint8_t data = '\0';
volatile uint8_t menu_trigger =0;
volatile uint8_t upload_trigger =0;

volatile uint32_t add_count_app1 = app1_add_start;
volatile uint32_t add_count_app2 = app2_add_start;
uint32_t received_data[] = {0x00000000};
uint32_t temp_data[14336]; //Ram memory

int main(void)
{
	GPIO_Config();
	USART_Config(9600);

		SendTxt("Starting System\n\r");
		for(int i = 0; i<=16384; i++)
		{
			temp_data[i] = 0xFFFFFFFF;
		}
		SysTick_Config(SystemCoreClock/1000);
		delay(5000);
		BootloaderIsTrigger();
		Bootloader_menu(BootloaderIsTrigger());
		goto_application();


	 while(1)
	    {
		 if((ok == 1) & (download_count != 0))
		 {
			 delay(999); //wait 1 second after upload
			 if(delay_count == 0)
			 {
				 switch((download_count/4)%4)
				 {
				 case 0:
					 flash_count=(download_count/4);
					 break;
				 case 1:
					 flash_count=(download_count/4)+3;
					 break;
				 case 2:
					 flash_count=(download_count/4)+2;
				 	 break;
				 case 3:
					 flash_count=(download_count/4)+1;
				 	 break;
				 }

				 switch(upload_trigger)
				 {
				 case '1':
					 if(Write_App_1_check==0)
						 Write_App_1(temp_data);
				 break;
				 case '2':
					 if(Write_App_2_check==0)
						 Write_App_2(temp_data);
				 break;
				 }
			 }
		 }
	    }
	 return 0;
}

void SysTick_Handler(void)
{
	if (delay_count > 0)
		delay_count--;
}

void USART_Config(uint32_t baud) 
{
	RCC->APB1LENR |= 0x40000;

	USART3->BRR = (uint16_t)(SystemCoreClock / baud);		// BaudRate
	USART3->CR1 |= 1 << 2;		// Rx enable PD9
	USART3->CR1 |= 1 << 3;		// Tx enable //PD8
	USART3->CR1 |= 1 << 5;		// uart interrupt
	USART3->CR1 |= 0 << 10;		// Parity control disable
	USART3->CR1 |= 0 << 12;		// Word length 8bit
	USART3->CR2 |= 0 << 12;		// Stop bit 1
	USART3->CR1 |= 1 << 0;		// Usart enable

	NVIC_EnableIRQ(USART3_IRQn);
	NVIC_SetPriority(USART3_IRQn , 1);

}
void GPIO_Config() 
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
void FLASH_UnLocker(uint8_t Bank)
{
	while ((FLASH->SR1 & 0x00000001) != 0 );	
	while ((FLASH->SR2 & 0x00000001) != 0 );	
	switch(Bank)
	{
	case 1:
			if(FLASH->CR1 & 0x00000001)				
			{
				FLASH->KEYR1 = 0x45670123;
				FLASH->KEYR1 = 0xCDEF89AB;
			}
		break;
	case 2:
			if(FLASH->CR2 & 0x00000001)				
			{
				FLASH->KEYR2 = 0x45670123;
				FLASH->KEYR2 = 0xCDEF89AB;
			}
		break;
	}
}

void FLASH_Locker(uint8_t bank)
{
	switch(bank)
	{
	case 1:
		FLASH->CR1 |= 0x80000000;
		break;
	case 2:
		FLASH->CR2 |= 0x80000000;
		break;
	}
}
void FLASH_Erase(uint8_t SER, uint8_t SSB)
{
	// Clear Flags
										//BANK 1
	FLASH->CCR1 |= 0x10000;				// End of Operation flag clear
	FLASH->CCR1 |= 0x40000;				// Programming Parallelism error flag clear
	FLASH->CCR1 |= 0x20000;				// Write protected error flag clear

										//BANK 2
	FLASH->CCR2 |= 0x10000;				// End of Operation flag clear
	FLASH->CCR2 |= 0x40000;				// Programming Parallelism error flag clear
	FLASH->CCR2 |= 0x20000;				// Write protected error flag clear

	while((FLASH->SR1 & 0x1) != 0);	
	while((FLASH->SR2 & 0x1) != 0);	

	if(SER == 1)
	{
		FLASH->CR1 |= 0x4;				// SER bit
		FLASH->CR1 |= SSB << 6;
		FLASH->CR1 |= 0x20;					
		while(FLASH->SR1 & 0x4);			
	}
	else if(SER == 2)
	{
		FLASH->CR2 |= 0x4;
		FLASH->CR2 |= SSB << 6;
		FLASH->CR2 |= 0x20;					
		while(FLASH->SR2 & 0x4);			
	}

	FLASH->CR1 &= 0x0000000 << 6;
	FLASH->CR2 &= 0x0000000 << 6;

}
uint8_t BootloaderIsTrigger()
{
	if(data == '\0')
		return 0;
	else
		return 1;
}
void Bootloader_menu(uint8_t BootloaderIsTrigger)
{
	if(BootloaderIsTrigger == 0)
		whichApp = 1;
	else
	{
		/////////////////////////////////////////////////////////////
		SendTxt("Please choose options: \n\r");
		SendTxt("	1) Go to application 1 \n\r");
		SendTxt("	2) Go to application 2 \n\r");
		SendTxt("	3) Delete application 1 \n\r");
		SendTxt("	4) Delete application 2 \n\r");
		SendTxt("	5) Upload mode \n\r");
		SendTxt("	6) Reset \n\r");
		/////////////////////////////////////////////////////////////
		menu_trigger=0;
		while(!menu_trigger);

		switch (data)
		{
		case '1':
			whichApp = 1;
			break;
		case '2':
			whichApp = 2;
			break;
		case '3':
			SendTxt("Deleting application 1 \n\r");
			Delete_App_1();
			SendTxt("DELETED !!\n\r");
			break;
		case '4':
			SendTxt("Deleting application 2 \n\r");
			Delete_App_2();
			SendTxt("DELETED !!\n\r");
			break;
		case '5':
			SendTxt("Please choose options: \n\r");
			ok=2;
			SendTxt("	1) Upload app 1 \n\r");
			SendTxt("	2) Upload app 2 \n\r");
			while(!upload_trigger);
			if(upload_trigger == '1')
			{
				ok=1;
				SendTxt("Upload your application \n\r");
				whichApp = 3;
			}

			else if(upload_trigger == '2')
			{
				ok=1;
				SendTxt("Upload your application \n\r");
				whichApp = 3;
			}
			else
			{
				SendTxt("Wrong Command !!\n\r");
				whichApp = 0;
			}
			break;
		case '6':
			whichApp = 0;
			break;
		default:
			SendTxt("Wrong Command !!\n\r");
			break;
		}
	}
}
static void goto_application()
{
	void (*boot_reset_handler) (void) = (void*) ( *(volatile uint32_t*) (0x08000000 + 4));
	void (*app_reset_handler1) (void) = (void*) ( *(volatile uint32_t*) (0x08040000 + 4));
	void (*app_reset_handler2) (void) = (void*) ( *(volatile uint32_t*) (0x08120000 + 4));
	switch(whichApp)
	{
		case 0:
			SendTxt("Resetting.. \n\r");
			delay(3000);
			boot_reset_handler();
			break;
		case 1:
			SendTxt("Jump to application 1 \n\r");
			app_reset_handler1();
			break;
		case 2:
			SendTxt("Jump to application 2 \n\r");
			app_reset_handler2();
			break;
		case 3:
			break;
		default:
			break;
	}
}
void Delete_App_1()
{
	for(int i = 32; i<64; i++)
	{
		FLASH_UnLocker(1);
		FLASH_Erase(1, i);
		FLASH_Locker(1);
	}

}
void Delete_App_2()
{
	for(int i = 16; i<48; i++)
	{
		FLASH_UnLocker(2);
		FLASH_Erase(2, i);
		FLASH_Locker(2);
	}
}


void FLASH_Write(uint32_t address, uint32_t data)
{

	while(FLASH->SR1 & 0x1);		
	FLASH->CR1 |= 0x2;
	while(FLASH->SR2 & 0x1);		
	FLASH->CR2 |= 0x2;	
	
	*(__IO uint32_t*)address =  data;

	while(FLASH->SR1 & 0x4);		
	while(FLASH->SR1 & 0x1);
	while(FLASH->SR2 & 0x4);		
	while(FLASH->SR2 & 0x1);
	FLASH->CCR1 |= 0x10000;
	if((FLASH->SR1 & 0x10000) != 0)	
	{
		FLASH->CCR1 |= 0x10000;		
		
	}
	if((FLASH->SR2 & 0x10000) != 0)	
	{
		FLASH->CCR2 |= 0x10000;		
		
	}
			
}

void Write_App_1(uint32_t *data)
{

	Delete_App_1();
	FLASH_UnLocker(1);
	while (say <= flash_count)
	{
		FLASH_Write(add_count_app1,data[say]);
		add_count_app1 = add_count_app1 + 4;
		if(add_count_app1==0x08080000);
		say++;
	}
	SendTxt("Upload Complete..");
	Write_App_1_check=1;
	FLASH->CR1 &= ~0x2;
	FLASH_Locker(1);
}
void Write_App_2(uint32_t *data)
{

	Delete_App_2();
	FLASH_UnLocker(2);
	while (say <= flash_count)
	{
		FLASH_Write(add_count_app2,data[say]);
		add_count_app2 = add_count_app2 + 4;
		say++;
	}
	SendTxt("Upload Complete..");
	Write_App_2_check=1;
	FLASH->CR1 &= ~0x2;
	FLASH_Locker(2);
}
void downloaded()
{
	received_data[0] |= data;
	if(down_i<4)
	{
		switch(down_i)
		{
		case 0:
			temp_data[down_k] &=0x00000000;
			temp_data[down_k] |= received_data[0];
			received_data[0] &=0x00000000;
			break;
		case 1:
			temp_data[down_k] &=0x000000FF;
			temp_data[down_k] |= received_data[0]<<8;
			received_data[0] &=0x00000000;
			break;
		case 2:
			temp_data[down_k] &=0x0000FFFF;
			temp_data[down_k] |= received_data[0]<<16;
			received_data[0] &=0x00000000;
			break;
		case 3:
			temp_data[down_k] &=0x00FFFFFF;
			temp_data[down_k] |= received_data[0]<<24;
			received_data[0] &=0x00000000;
			break;
		}
			down_i++;
	}

	if(down_i==4) 
	{
		down_i=0;
		down_k++;
	}
}
void delay(volatile uint32_t second)
{
	delay_count = second;
	menu_trigger=0;
    while(delay_count)
    	{
    	if(delay_count == 5000)
    		SendTxt("Time left to start bootloader (press any key to start): 5 \n\r");
   		else if(delay_count == 4000)
   			SendTxt("4\n\r");
    	else if(delay_count == 3000)
    		SendTxt("3\n\r");
    	else if(delay_count == 2000)
    		SendTxt("2\n\r");
    	else if(delay_count == 1000)
    		SendTxt("1\n\r");

    	if(menu_trigger)
    		break;
    	}
}
void USART3_IRQHandler(void)
{
	data = (uint8_t)USART3->RDR;
	menu_trigger = 1;
	USART3->ICR |= 0x8; //ORE flag
	USART3->ICR |= 0x1000;	//EOBF flag
	if(ok==1)
	{
		downloaded();
		download_count++;
	}
	else if(ok==2)
		upload_trigger = data;
}

