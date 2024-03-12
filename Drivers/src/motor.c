/*
 * motor.c
 *
 * Created: 11/24/2022 7:09:28 PM
 *  Author: dell
 */ 
#include "motor.h"

//Initialization of motor
void motor_init()
{
	MOTOR_DDR |=(1<<CW_PIN);
	MOTOR_DDR |=(1<<ANTI_CW_PIN);

}
//Rotate the motor
void motor_rotate(EN_rotation_t rot)
{
	switch (rot)
	{
		case clockwise:
			MOTOR_REG &=~(1<<ANTI_CW_PIN);
			MOTOR_REG |=(1<<CW_PIN);
			break;
		case anticlockwise:
			MOTOR_REG &=~(1<<CW_PIN);
			MOTOR_REG |=(1<<ANTI_CW_PIN);
			break;
		default:
			break;
	}
}
//Stop the motor
void motor_stop()
{
	MOTOR_REG &=~(1<<ANTI_CW_PIN);
	MOTOR_REG &=~(1<<CW_PIN);

}
