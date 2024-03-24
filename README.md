# Smart-home-based-on-RTOS
This project is the final project of RTOS in NTI, this project is based on RTOS and Atmega32
### The features of this project:
1- Control Fans and AC (air conditioner).

2- Control the rooms' lights.

3- Door security access.

#### Control Fans and AC (air conditioner):
- Fans will be turned on if the temperature is greater than or equal to 30 and less than or equal to 35.
- AC will be turned on  if the temperature is less than 35.

#### Control the rooms' lights:

The rooms' lights will be  turned on by frame structure:

- if I enter on the UART * I * 1 * 1 #  ,the room1's led  will be turned on.
- if I enter on the UART * I * 2 * 0 #  ,the room2's led  will be turned off.
- 'I' refers to the room's light, and the number next to 'I' refers to the room number
- if the last number is 0 the room's LED will turn off.
- if the last number is 1 the room's LED will turn on.

#### Door security access:

- if the user enters on the UART * A *  1 #,  the user will enter the  right password and the door will be opened 
- if the user enters on the UART * A *  0 #,   the door will be closed.
- if the user enters on the UART * P #, the user will change the password and make a new password.


### Hardware design:



  


    

