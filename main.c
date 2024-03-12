/*
 * main.c
 *
 *  Created on: Mar 27, 2023
 *      Author: dell
 */


/*FreeRTOS includes*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
/*Drivers includes*/
#include "uart.h"
#include "BasicIO.h"
#include "LCD.h"
#include "motor.h"
#include "ADC.h"

/*prototypes*/
void system_init(void);
void T_TempDetect(void *pvParam);
void T_TempControl(void *pvParam);
void T_Uart(void *pvParam);
void T_RoomLeds(void *pvParam);
void T_Door(void *pvParam);
void T_NewPassWord(void *pvParam);

/* Event group Declaration */
EventGroupHandle_t egSmartHome=NULL;
EventBits_t egValue = NULL;
#define E_Fan      (1<<0)
#define E_AC       (1<<1)
#define E_ILL      (1<<2)
#define E_Door     (1<<3)
#define E_Stop     (1<<4)
#define E_PassWord      (1<<5)



void main()
{
	/*Pr. init*/
	system_init();
	/* Create OS Objects*/
	egSmartHome = xEventGroupCreate();
	/*Create Tasks*/
	xTaskCreate(T_TempDetect,"T_TempDetect",120,NULL,1,NULL);
	xTaskCreate(T_TempControl,"T_TempControl",120,NULL,2,NULL);
	xTaskCreate(T_Uart,"T_Uart",120,NULL,3,NULL);
	xTaskCreate(T_RoomLeds,"T_RoomLeds",120,NULL,4,NULL);
	xTaskCreate(T_Door,"T_Door",120,NULL,5,NULL);
	xTaskCreate(T_NewPassWord,"T_NewPassWord",120,NULL,6,NULL);

	/* Starts OS*/
	vTaskStartScheduler();
	/*Unreachable code*/
	while(1);

}

void system_init(void)
{
	/* Initiate UART */
	Uart_Init(9600);
	/* Initiate LEDs */
	Leds_AllInit();
	/* Initiate Motor */
	motor_init();
	/* Initiate ADC */
	ADC_Init();
	/* Send Start mssg on UART */
	Uart_SendStr("System Start\r\n");
}

/* Task to detect temperature */
void T_TempDetect(void *pvParam)
{
	/* Uart send massage on UART */
	Uart_SendStr("T_TempDetect\r\n");
	/* Variable store value of temp */
	uint32_t t_current=0;
	/* Store ADC value */
	uint32_t  LM35_value=0;

	while(1)
	{
		/* Check temp if temprature <30 */
		if((t_current < 30))
		{
			/*Clear bit E_Fan */
			xEventGroupClearBits(egSmartHome, E_Fan);
			/* Set E_stop to turn off leds & AC */
			xEventGroupSetBits(egSmartHome, E_Stop);

			while((t_current < 30))
			{
				/* Read adc value */
				LM35_value = ADC_Read(ADC_CH0);
				/* Store new value of temp */
				t_current = ((500*LM35_value)/1023);
				/* OS Delay Task Goto wait */
				vTaskDelay(50);
			}
		}
		/* Check temp if (temp>=30&&<=35) */
		else if((t_current >= 30) && (t_current <= 35))
		{
			/* Clear bits of event stop E_Stop */
			xEventGroupClearBits(egSmartHome, E_Stop);
			/* Clear bits of event AC E_AC */
			xEventGroupClearBits(egSmartHome, E_AC);
			/* Set bits of event Fan E_Fan */
			xEventGroupSetBits(egSmartHome, E_Fan);
			while((t_current >= 30) && (t_current <= 35))
			{
				/* Read adc value */
				LM35_value = ADC_Read(ADC_CH0);
				/* Store new value of temp */
				t_current = ((500*LM35_value)/1023);
				/* OS Delay Task Goto wait */
				vTaskDelay(50);
			}
		}
		/* Check temp if temp>35 */
		else if( t_current > 35 )
		{
			/* Clear E_stop bit */
			xEventGroupClearBits(egSmartHome, E_Stop);
			/* Clear bits of event Fan E_Fan */
			xEventGroupClearBits(egSmartHome, E_Fan);
			/* Set bits of event AC E_AC */
			xEventGroupSetBits(egSmartHome, E_AC);
			while(t_current > 35)
			{
				/* Read adc value */
				LM35_value = ADC_Read(ADC_CH0);
				/* Store new value of temp */
				t_current = ((500*LM35_value)/1023);
				/* OS Delay Task Goto wait */
				vTaskDelay(50);
			}
		}
		/* After user finish sending --> task go to wait */
		/* Other tasks can work now */
		vTaskDelay(50);
	}
}

/* Task to Control temperature */
void T_TempControl(void *pvParam)
{
	/* Uart send massage on UART */
	Uart_SendStr("T_TempControl\r\n");
	while(1)
	{
		/* Task work if the E_fan bit or E_AC or E_STOP in event group  */
		egValue =   xEventGroupWaitBits(egSmartHome,
				( E_Fan|E_AC|E_Stop),
				0,   //  1 ->clear event enable    0-> clear events disable
				0,  // 0->ORing between events if one event happen actions turn on   1-> Anding all Events must be happened
				portMAX_DELAY);
		/* Check if the E_fan bit set in event group  */
		if((egValue&(E_Fan|E_AC|E_Stop))== (E_Fan))
		{
			/* Turn off AC */
			Led_Off(LED4);
			/* Turn on Fan */
			Led_On(LED5);
			/* OS Delay Task Goto wait */
			vTaskDelay(50);
		}
		/* Check if the E_AC bit set in event group */
		else if((egValue&(E_Fan|E_AC|E_Stop))== (E_AC))
		{
			/* Turn on AC */
			Led_On(LED4);
			/* Turn off Fan */
			Led_Off(LED5);
			/* OS Delay Task Goto wait */
			vTaskDelay(50);
		}
		/* Check if the E_stop bit set in event group */
		else if((egValue&(E_Fan|E_AC|E_Stop))== (E_Stop))
		{
			/* Turn off AC */
			Led_Off(LED4);
			/* Turn off Fan */
			Led_Off(LED5);
			/* OS Delay Task Goto wait */
			vTaskDelay(50);
		}
	}
}

/* UART Task to forward the user to the task he want
   depending on what he send on terminal */
void T_Uart(void *pvParam)
{
	/* Define a local variable
	   to save the recieved charater from UART in */
	uint8_t data = 0;
	/* Task sends the initialization mssg on UART */
	Uart_SendStr("UART\r\n");
	while(1)
	{
		/* Recieve the data from user
		   and don't wait till he send it so other tasks can operate */
		Uart_ReceiveByte_Unblock(&data);
		/* won't proceed in the task untill user sends '*' */
		if(data == '*')
		{
			/* Receive data from user and wait untill he send it */
			Uart_ReceiveByte(&data);
			/* Switch on the character user sent to know which task user want */
			switch(data){
			/* If user sent I */
			case 'I':
				/* Set Event group bit for Illumination(RoomLeds) task */
				xEventGroupSetBits(egSmartHome, E_ILL);
				break;
				/* If user sent A */
			case 'A':
				/* Set Event group bit for Access door task */
				xEventGroupSetBits(egSmartHome, E_Door);
				break;
				/* If user sent P */
			case 'P':
				/* Set Event group bit for Change password task */
				xEventGroupSetBits(egSmartHome, E_PassWord);
				break;
			default:
				/* Do nothing */
				break;

			}
		}
		/* After user finish sending --> task go to wait */
		/* Other tasks can work now */
		vTaskDelay(50);
	}
}

/* RoomLeds task to turn on or off the LEDs of any room the user choose */
/* To turn on or off room leds send
   --> *I*RoomNumber*1 for on
   --> *I*RoomNumber*0 for off */
void T_RoomLeds(void *pvParam)
{
	/* Define a local variable
	   to save the recieved charater from UART in */
	uint8_t rdata=0;
	/* Task sends the initialization mssg on UART */
	Uart_SendStr("T_RoomLeds\r\n");
	while(1)
	{
		/* This task will work only if the ILL bit in event group is set
		   and will clear it after it's done */
		egValue =   xEventGroupWaitBits(egSmartHome,
				( E_ILL),
				1,//  1 ->clear event enable    0-> clear events disable
				1, // 0->ORing between events if one event happen actions turn on   1-> Anding all Events must be happened
				portMAX_DELAY);
		/* Check that the event bit is set */
		if((egValue&(E_ILL))== (E_ILL)){
			/* Receive the data from user and wait till he send it */
			Uart_ReceiveByte(&rdata);
			/* Proceed in the code if user sent '*' */
			if(rdata == '*')
			{
				/* Receive the data from user and wait till he send it */
				Uart_ReceiveByte(&rdata);
				/* Switch on the data to determine which room the user entered */
				switch(rdata){
				/* If user entered 1 --> Room 1  */
				case '1':
					/* Receive the data from user */
					Uart_ReceiveByte(&rdata);
					/* Proceed in the code if user sent '*' */
					if(rdata == '*')
					{
						/* Receive the data from user */
						Uart_ReceiveByte(&rdata);
						/* Switch on the data to determine whether to turn on or off Room 1 LED */
						switch(rdata){
						/* If user sent 1 --> to turn on the LED */
						case '1':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn on Room 1 LED */
								Led_On(LED1);
							}
							break;
							/* If user sent 0 --> to turn off the LED */
						case '0':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn off Room 1 LED */
								Led_Off(LED1);
							}
							break;
						}
					}
					break;
					/* If user entered 2 --> Room 2  */
				case '2':
					/* Receive the data from user */
					Uart_ReceiveByte(&rdata);
					/* Proceed in the code if user sent '*' */
					if(rdata == '*')
					{
						/* Receive the data from user */
						Uart_ReceiveByte(&rdata);
						/* Switch on the data to determine whether to turn on or off Room 2 LED */
						switch(rdata){
						/* If user sent 1 --> to turn on the LED */
						case '1':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn on Room 2 LED */
								Led_On(LED2);
							}
							break;
							/* If user sent 0 --> to turn off the LED */
						case '0':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn off Room 2 LED */
								Led_Off(LED2);
							}
							break;
						}
					}
					break;
					/* If user entered 3 --> Room 3  */
				case '3':
					/* Receive the data from user */
					Uart_ReceiveByte(&rdata);
					/* Proceed in the code if user sent '*' */
					if(rdata == '*')
					{
						/* Receive the data from user */
						Uart_ReceiveByte(&rdata);
						/* Switch on the data to determine whether to turn on or off Room 3 LED */
						switch(rdata){
						/* If user sent 1 --> to turn on the LED */
						case '1':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn on Room 3 LED */
								Led_On(LED3);
							}
							break;
							/* If user sent 0 --> to turn off the LED */
						case '0':
							/* Receive the data from user */
							Uart_ReceiveByte(&rdata);
							/* Don't proceed if user didn't send '#' */
							if(rdata == '#'){
								/* Turn off Room 3 LED */
								Led_Off(LED3);
							}
							break;
						}
					}
					break;
				default:
					/* Do nothing */
					break;
				}
				/* Write new line on UART terminal */
				Uart_SendStr("\r\n");
			}
		}
		/* After user finish sending --> task go to wait */
		/* Other tasks can work now */
		vTaskDelay(50);
	}
}
/* Global array , right password is 123 */
char passWord[10]="123";
/* Task to open and close door */
void T_Door(void *pvParam)
{
	/* Ddata is variable , it is used to receive data from UART */
	uint8_t Ddata=0;
	/* Check password array is used to enter password and check */
	char chPass[10]={0};
	/* i is variable,it is used when enter password */
	char i=0;
	/* j is variable,it is  used to check The number of attempts to enter the password */
	char j=0;
	/* Send on UART "T_Door" */
	Uart_SendStr("T_Door\r\n");
	while(1)
	{
		/* Task door wait on event Door bit */
		egValue =   xEventGroupWaitBits(egSmartHome,
				( E_Door),
				1,//  1 ->clear event enable    0-> clear events disable
				1, // 0->ORing between events if one event happen actions turn on   1-> Anding all Events must be happened
				portMAX_DELAY);
		/* Check if event door bit is set */
		if((egValue&(E_Door))== (E_Door)){
			/* Receive data from Uart terminal */
			Uart_ReceiveByte(&Ddata);
			/* Check if Ddata is '*' */
			if(Ddata == '*')
			{
				/* Recive data from UART terminal */
				Uart_ReceiveByte(&Ddata);
				/* Switch on UART terminal receive */
				switch(Ddata){
				/*Check if Ddata receive is '1'*/
				case '1':
					/* Receive data from UART terminal */
					Uart_ReceiveByte(&Ddata);
					/* Check if Ddata is '#' */
					if(Ddata == '#'){
						/* Use do_ while condition to enter password */
						do {
							/* Send on Uart Enter Password */
							Uart_SendStr("Enter PassWord\r\n");
							/* Receive data from UART terminal */
							Uart_ReceiveByte(&Ddata);
							/* Check if Ddata not equal 13(this mean Enter button in Keyboard is not pressed) */
							while(Ddata !=13)
							{
								/* The first element of array chPass is equal the data receive from UART */
								chPass[i]=Ddata;
								/* The next element of array chPass is Null */
								chPass[i+1]=NULL;
								/* Receive data again from the UART terminal */
								Uart_ReceiveByte(&Ddata);
								/* After receive data increment i */
								i++;
							}
							/* After receive Password make i equal 0 to make user to open door again */
							i=0;
							/* Increment j to make the user enter password again if password is wrong */
							j++;
							/* Check if the password receive from the user  is wrong and check if number of attempts is not less than3 */
						} while ((strcmp(chPass,passWord) != 0) &&(j!=3));
						/* Check if password receive from user is right */
						if(strcmp(chPass,passWord) != 0){
							Uart_SendStr("Wrong PassWord\r\n");
						}
						if(strcmp(chPass,passWord) == 0){
							/* Open door by rotate the motor clockwise */
							motor_rotate(clockwise);
							vTaskDelay(500);
							motor_stop();
						}

					}
					/* After  enter the right password make j ( The number of attempts to enter the password)equal 0 */
					j=0;
					break;
					/* Check if data receive from UART is '0' */
				case '0':
					/* Receive data from UART terminal */
					Uart_ReceiveByte(&Ddata);
					/* Check if data from UART is '#" */
					if(Ddata == '#'){
						/* Close Door By Make motor rotate anticlockwise */
						motor_rotate(anticlockwise);
						vTaskDelay(500);
						motor_stop();
					}
					break;
				default:
					/* Do nothing */
					break;
				}
				/* Write new line on UART terminal */
				Uart_SendStr("\r\n");
			}
		}
		/* After user finish sending --> task go to wait */
		/* Other tasks can work now */
		vTaskDelay(50);
	}
}

/* Task To Enter new password */
/* To change the password send*/
void T_NewPassWord(void *pvParam)
{
	/* Pdata is variable , it is used to receive data from UARTt*/
	uint8_t Pdata=0;
	/* Check password array is used to enter password and check*/
	char chPass[10]={0};
	/*NewPass1 array is used to enter new  password*/
	char NewPass1[10]={0};
	/*NewPass2 array is used to enter new  password again*/
	char NewPass2[10]={0};
	/* i is variable,it is used when enter password */
	char i=0;
	/* j is variable,it is  used to check The number of attempts to enter the password */
	char j=0;
	Uart_SendStr("T_NewPassWord\r\n");
	while(1)
	{
		/* Task new password wait on event passWord bit */
		egValue =   xEventGroupWaitBits(egSmartHome,
				( E_PassWord),
				1,//  1 ->clear event enable    0-> clear events disable
				1, // 0->ORing between events if one event happen actions turn on   1-> Anding all Events must be happened
				portMAX_DELAY);
		/* Check if event passWord bit is set */
		if((egValue&(E_PassWord))== (E_PassWord)){
			/* Receive data from UART terminal */
			Uart_ReceiveByte(&Pdata);
			if(Pdata == '#')
			{
				do {
					/* Send on UART enter passWord */
					Uart_SendStr("Enter PassWord\r\n");
					/* Receive data from UART terminal */
					Uart_ReceiveByte(&Pdata);
					/* Check if Pdata not equal 13(this mean Enter button in Keyboard is not pressed) */
					while(Pdata !=13)
					{
						/* The first element of array chPass is equal the data receive from UART */
						chPass[i]=Pdata;
						/* The next element of array chPass is Null */
						chPass[i+1]=NULL;
						/* Receive data again from the UART terminal */
						Uart_ReceiveByte(&Pdata);
						/* After receive data increment i */
						i++;
					}
					/* After receive Password make i equal 0 to make user to enter new password */
					i=0;
					/* Increment j to make the user enter password again if password is wrong */
					j++;
					/* Check if the password receive from the user  is wrong and check if number of attempts is not less than  3 */
				} while ((strcmp(chPass,passWord) != 0) &&(j!=3));
				/* After  enter the right password make j ( The number of attempts to enter the password)equal 0 */
				j=0;
				if(strcmp(chPass,passWord) != 0){
					Uart_SendStr("Wrong PassWord\r\n");
				}
				/* Check if password receive from user is right */
				if(strcmp(chPass,passWord) == 0){
					/* Send on UART new PassWord */
					Uart_SendStr("New PassWord\r\n");
					/* Receive data from UART terminal */
					Uart_ReceiveByte(&Pdata);
					/* Check if Pdata not equal 13(this mean Enter button in Keyboard is not pressed) */
					while(Pdata !=13)
					{
						/* The first element of array NewPass1 is equal the data receive from UART */
						NewPass1[i]=Pdata;
						/* The next element of array NewPass1 is Null */
						NewPass1[i+1]=NULL;
						/* Receive data again from the UART terminal */
						Uart_ReceiveByte(&Pdata);
						/* After receive data increment i */
						i++;
					}
					/* After receive Password make i equal 0 to make user to reenter new password again */
					i=0;
					/* Send on UART Reenter new password */
					Uart_SendStr("Reenter New PassWord\r\n");
					/* Receive data from Uart */
					Uart_ReceiveByte(&Pdata);
					/* Check if Pdata not equal 13(this mean Enter button in Keyboard is not pressed) */
					while(Pdata !=13)
					{
						/* The first element of array NewPass2 is equal the data receive from UART */
						NewPass2[i]=Pdata;
						/* The next element of array NewPass2 is Null */
						NewPass2[i+1]=NULL;
						/* Receive data again from the UART terminal */
						Uart_ReceiveByte(&Pdata);
						/* After receive data increment i */
						i++;
					}
					/* After receive Password make i equal 0 to make user to reenter new password again */
					i=0;
					/* Check if  Newpass1 and Newpass2 is equal */
					if(strcmp(NewPass1,NewPass2) == 0)
					{
						/* Update password */
						strcpy(passWord,NewPass2);
						/* send on UART password is Updated*/
						Uart_SendStr("password is update\r\n");
						/* Display new passWord on UART */
						Uart_SendStr(passWord);
						Uart_SendStr("\r\n");
					}
					/* Check if NewPass1 is not equal Newpass2 */
					else if(strcmp(NewPass1,NewPass2) != 0)
					{
						/* Send on UART passwords are not the same */
						Uart_SendStr("passwords are not the same \r\n");
						/* Send on UART Reenter New PassWord again*/
						Uart_SendStr("Reenter New PassWord again\r\n");
						/* Receive new password again*/
						/* Receive data from UART */
						Uart_ReceiveByte(&Pdata);
						/* Check if Pdata not equal 13(this mean Enter button in Keyboard is not pressed) */
						while(Pdata !=13)
						{
							/* The first element of array NewPass2 is equal the data receive from UART*/
							NewPass2[i]=Pdata;
							/* The next element of array NewPass2 is Null */
							NewPass2[i+1]=NULL;
							/* Receive data again from the UART terminal */
							Uart_ReceiveByte(&Pdata);
							/* After receive data increment i */
							i++;
						}
						/* After receive Password make i equal 0 */
						i=0;
						/* Check if  Newpass1 and Newpass2 is equal */
						if(strcmp(NewPass1,NewPass2) == 0)
						{
							/* Update password */
							strcpy(passWord,NewPass2);
							/* send on UART password is Update */
							Uart_SendStr("password is update\r\n");
							/* Display new passWord on UART*/
							Uart_SendStr(passWord);
							Uart_SendStr("\r\n");
						}
					}
				}
				/* Write new line on UART terminal */
				Uart_SendStr("\r\n");
			}
		}
		/* After user finish sending --> task go to wait */
		/* Other tasks can work now */
		vTaskDelay(50);
	}
}
