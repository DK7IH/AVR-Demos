There are 3 version to read a rotary encoder. 

Version 2 is more simple than V1. If it does not work from scratch, try to change the encoder pins 
A and B since there is only 1 interrupt triggering pin. If it is the "wrong" one only one 
direction will be detected!

V3 in addtion uses Timer1 as counter for Milliseconds and resets LEDs 1s after the last took place
