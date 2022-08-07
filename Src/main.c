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
void Delete_App_2();
void CRC_Sequence_for_app1();
void crc_init_16();
void downloaded();


uint32_t final_data[] = {0x00000000,0x00000000,0x00000000,0x00000000};

volatile uint8_t down_k=0;
volatile uint8_t down_i=0;
volatile uint8_t temp =0;
volatile uint8_t say =0;
volatile uint8_t sayıcı =0;
volatile uint8_t ok =0;
volatile uint32_t count;
volatile uint8_t whichApp ='\0';
volatile uint8_t data = '\0';
volatile uint8_t flag =0;
volatile uint32_t crc_data;
#define BOYUT 262144U
#define app1_add_start 0x08040000
#define app1_add_end 0x08080000
#define app2_add 0x08120000
volatile uint32_t add_count_app1 = app1_add_start;
uint32_t received_data[] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF}; //eop bitir wnbe check et


void delay(volatile uint32_t second) {
	count = second;
    while(count);
}

int main(void)
{
	GPIO_Config();
	USART_Config(9600);
	crc_init_16();
	//CRC_Sequence_for_app1();

		 SendTxt("Starting System\n\r");
		 SysTick_Config(SystemCoreClock/1000);
		 delay(7000);
		 BootloaderIsTrigger();
		Bootloader_menu(BootloaderIsTrigger());
		goto_application();


	 while(1)
	    {
	    }
	 return 0;
}

void SysTick_Handler(void)
{
	if(count == 5000)
		SendTxt("Time left to start bootloader (press any key to start): \n\r\n\r");
	else if(count == 4000)
		SendTxt("4\n\r");
	else if(count == 3000)
		SendTxt("3\n\r");
	else if(count == 2000)
		SendTxt("2\n\r");
	else if(count == 1000)
		SendTxt("1\n\r");
	if (count > 0)
		count--;
}

void USART_Config(uint32_t baud) //Bitti
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
void FLASH_UnLocker(uint8_t Bank)
{
	while ((FLASH->SR1 & 0x00000001) != 0 );	// Flash Bank1 belleğin meşgul olmamasını bekle
	while ((FLASH->SR2 & 0x00000001) != 0 );	// Flash Bank2 belleğin meşgul olmamasını bekle
	switch(Bank)
	{
	case 1:
			if(FLASH->CR1 & 0x00000001)				// Flas Bank1'in kilidinin açık olup olmadığını kontrol et  LOCK1 registeri kontrol edilir
			{
				FLASH->KEYR1 = 0x45670123;
				FLASH->KEYR1 = 0xCDEF89AB;
			}
		break;
	case 2:
			if(FLASH->CR2 & 0x00000001)				// Flaş Bank2'in kilidinin açık olup olmadığını kontrol et  LOCK2 registeri kontrol edilir
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
	// Bayrakları temizle
										//BANK 1
	FLASH->CCR1 |= 0x10000;				// End of Operation flag clear
	FLASH->CCR1 |= 0x40000;				// Programming Parallelism error flag clear
	FLASH->CCR1 |= 0x20000;				// Write protected error flag clear

										//BANK 2
	FLASH->CCR2 |= 0x10000;				// End of Operation flag clear
	FLASH->CCR2 |= 0x40000;				// Programming Parallelism error flag clear
	FLASH->CCR2 |= 0x20000;				// Write protected error flag clear

	while((FLASH->SR1 & 0x1) != 0);	// Meşguliyet bitene kadar bekle
	while((FLASH->SR2 & 0x1) != 0);	// Meşguliyet bitene kadar bekle

	if(SER == 1)
	{
		FLASH->CR1 |= 0x4;				// sektör silme bitini ayarla Bank'a göre (SER)
		FLASH->CR1 |= SSB << 6;
	}
	else if(SER == 2)
	{
		FLASH->CR2 |= 0x4;
		FLASH->CR2 |= SSB << 6;
	}
	FLASH->CR1 |= 0x20;					// Start biti
	while(FLASH->SR1 & 0x4);			// İşlemler bitene kadar bekle

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
		SendTxt("	5) Download mode \n\r");
		SendTxt("	6) Reset \n\r");
		/////////////////////////////////////////////////////////////

		while(!(USART3->ISR & 0x20));

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
			SendTxt("Upload your application \n\r");
			ok=1;
				whichApp = 3;
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
			//SendTxt("Upload complete.. \n\r");
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
/*void nondef()
{
	uint16_t all_bytes = 0x00;

	for (i = 3; i >= 1; i--) {
		all_bytes |= dummy_array[i] << (8 * i);
	}
	all_bytes |= dummy_array[0];
	CRC->DR = all_bytes;
	uint32_t crc_code = CRC->DR;
	uart2_send(((crc_code) & 0xFF));
	uart2_send(((crc_code >> 8) & 0xFF));
	uart2_send(((crc_code >> 16) & 0xFF));
	uart2_send(((crc_code >> 24) & 0xFF));

}*/
void crc_init_16()
{
	RCC->AHB1ENR |= 0x200;  // enable clock for CRC
 	CRC->CR |= CRC_CR_RESET;   // Reset calculation
 	CRC->CR = 1 << 3; // set poly to 16 bit
 	CRC->POL = 0x4C11DB7;     // pick a random poly
 	CRC->INIT = 0xFFFF;     //init value also 16 bit
}
void CRC_Sequence_for_app1()
{
	FLASH_UnLocker(1);
	FLASH->CR1 |= 0x8000; // CRC_EN bit
	FLASH->CRCCR1 |= 0x200000;
	FLASH->CRCSADD1 |= 0x08040000 << 2; //CRC başlangıç adresi
	FLASH->CRCEADD1 |= 0x08080000 << 2; //CRC bitiş adresi

	FLASH->CRCCR1 |= 10000; //CRC Start biti
	while ((FLASH->SR1 & 0x8) != 0 ); //CRC Busy bekle
	crc_data = (uint32_t)FLASH->CRCDATA;
	FLASH_Locker(1);
}

void FLASH_Write(uint32_t address, uint32_t data)
{

	while(FLASH->SR1 & 0x1);		// Meşguliyet bitene kadar bekle
	FLASH->CR1 |= 0x2;			// PG biti ayarlandı, Programmin modda		*(__IO uint32_t*)address = data;	// İstenilen adrese istenilen veri yazılıyor.
	//while((FLASH->SR1 & 0x2) == 0)
		*(__IO uint32_t*)address =  data;

	while(FLASH->SR1 & 0x4);		// Meşguliyet bitene kadar bekle
	while(FLASH->SR1 & 0x1);
	FLASH->CCR1 |= 0x10000;
	if((FLASH->SR1 & 0x10000) != 0)	// İşlem sonu biti ayarlandı ise
	{
		FLASH->CCR1 |= 0x10000;		// İşlem başarılı oldu, işlem sonu bağrağı temizle
		//*(__IO uint32_t*)address = data;
	}
			// CR register'ı PG biti başlangıçtaki duruma ayarla
}

void Write_App_1(uint32_t *data)
{

	FLASH_UnLocker(1);
	while (say <= 3)
	{
		FLASH_Write(add_count_app1,data[say]);
		add_count_app1 = add_count_app1 + 4;
		say++;
	}
	say = 0;
	FLASH->CR1 &= ~0x2;
	FLASH_Locker(1);
}
void downloaded()
{
	if(down_i<4)
	{
			received_data[down_i] = data;
			down_i++;
	}

	if(down_i==4)
	{
			final_data[down_k] |= received_data[0];
			final_data[down_k] |= received_data[1]<<8;
			final_data[down_k] |= received_data[2]<<16;
			final_data[down_k] |= received_data[3]<<24;
			down_i=0;
			down_k++;
			received_data[0] &= 0xFFFFFFFF;
			received_data[1] &= 0xFFFFFFFF;
			received_data[2] &= 0xFFFFFFFF;
			received_data[3] &= 0xFFFFFFFF;
			if(down_k==4)
			{
				Write_App_1(final_data);
				down_k=0;
				final_data[0] = 0x00000000;
				final_data[1] = 0x00000000;
				final_data[2] = 0x00000000;
				final_data[3] = 0x00000000;
			}
	}
}

void USART3_IRQHandler(void){
	data = (uint8_t)USART3->RDR;
	USART3->ICR |= 0x8; //ORE flag
	USART3->ICR |= 0x1000;	//EOBF flag
	if(ok==1)
		downloaded();
}

