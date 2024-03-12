/*
 * motor.h
 *
 * Created: 11/24/2022 7:09:14 PM
 *  Author: dell
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_
#include "avr/io.h"
#include "data_types.h"

#define  MOTOR_REG  PORTB
#define  MOTOR_DDR    DDRB
#define  CW_PIN 0
#define  ANTI_CW_PIN 1

typedef enum{
	clockwise,
	anticlockwise
	
	}EN_rotation_t;

void motor_init();
//void motor_rotate_clockwise();
//void motor_rotate_anticlockwise();
void motot_stop();
void motor_rotate(EN_rotation_t rot);


#endif /* MOTOR_H_ */
