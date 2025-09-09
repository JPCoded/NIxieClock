/* nixie10.h
* A library for driving 10-number nixie tubes with Arduino and various circuit configurations
* Chris Green - 1/9/2016
*/

#ifndef nixie10_h
#define nixie10_h

#include "Arduino.h" //needed for Arduino-specific functions

class nixie10{
public:
	//initializers for sn74141 driver configuration
	void initialize_sn74141(int pins[4], int initVal); //initializes tube with given value
	void initialize_sn74141(int pins[4]); //initializes tube to zero
	void clear_sn74141(); //makes the tube display nothing
	void set_sn74141(int digit); //sets the tube to a 1-digit number
	//initializers for 16-bit shift-register -> 4x sn74141 driver
	void initialize_16reg(int latchPin, int clockPin, int dataPin); //these are pins connecting the 16-bit register to Arduino
	void set_16reg(int ten_hour, int one_hour, int ten_minute, int one_minute); //set all tube values
private:
	void _my_shift_out(int dataPin, int clockPin, int val); //my own implementation of Arduino's shiftOut(), 1 bit only;
	void _dec_to_bcd(int decimal, int *bcd); //converts a decimal to a bcd string
	char _bcd[4]; //holds the current bcd output
	char _pins[4]; //holds the 4 Arduino digital pins to drive the tube
	int _latchPin, _clockPin, _dataPin; //pinout for a shift register configuration
};

#endif
  