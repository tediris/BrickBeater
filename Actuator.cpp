#include "Arduino.h"
#include "Actuator.h"

Actuator::Actuator(int pin){
	_mpin = pin;
}

void Actuator::init(int min, int max){
	_servo.attach(_pin);
	_min = min;
	_max = max;
}

void Actuator::extend(){
	_servo.write(_max);
}

void Actuator::retract(){
	_servo.write(_min);
}