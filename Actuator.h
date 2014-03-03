#ifndef Actuator_h
#define Actuator_h

#include "Arduino.h"
#include "Servo.h"

class Actuator {
public:
	Actuator(int pin);
	void init(int min, int max);
	void extend();
	void retract();

private:
	Servo _servo;
	int _pin;
	int _min;
	int _max;
};

#endif