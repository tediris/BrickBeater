
// BrickBeater.ino

#include "Timer.h"
#include "Drive.h"
#include "NewPing.h"
#include "Servo.h"


//STATES
#define SCANNING_FIRST_BEACON 1
#define SPINNING_FIRST_BEACON 2
#define HIT_FIRST_VINYL 3
#define SCANNING_SECOND_BEACON 4
#define SPINNING_SECOND_BEACON 5
#define HIT_SECOND_VINYL 6
#define SCANNING_SERVER 7
#define SPINNING_SERVER 8
#define HIT_SERVER 9
#define MINING 10
#define DEAD 11

//PINS
#define LEFT A5
#define MIDDLE A6
#define RIGHT A7

//BEACON READ VALUES
#define BEACON_CENTER 0
#define BEACON_LEFT 1
#define BEACON_RIGHT 2
#define NO_BEACON 3
#define AVG_MINIMUM 130

#define VINYL A0
#define MIN_DIST 20

#define NUM_TURNS 5
#define LEFT_SIDE 0
#define RIGHT_SIDE 1

#define BASE_SPEED 210

Timer spinTimer(250);
Timer scanTimer(500);
Timer arcTimer(350);
Timer missedFirstBeaconTimer(1000);

NewPing sonar(5, 6, 100);

Drive drive(210, 13, 12, 11, 3);
Actuator miner(9);

//Servo miner;

int state = SCANNING_FIRST_BEACON;
int fieldSide = -1;
int fieldSideCounter = 0;
int approxVinyl = -1;

int hitCounter = 0;

void setup() {
	//miner.attach(9);
	miner.init(80, 10);
	drive.init(LOW, HIGH);
	scanTimer.start();
	pinMode(A0, INPUT);
	pinMode(LEFT, INPUT);
	pinMode(MIDDLE, INPUT);
	pinMode(RIGHT, INPUT);
	approxVinyl = analogRead(VINYL) - 200;
}


void loop() {


	int avg = 0;
	int pos = NO_BEACON;
	int wallDist = 0;

	switch (state){

		case SCANNING_FIRST_BEACON:
			//drive.stop();
			
			avg = getAverageBeaconValue();
			if (avg > AVG_MINIMUM){
				state = HIT_FIRST_VINYL;
				missedFirstBeaconTimer.start();
				break;
			}
			if (scanTimer.isExpired()){
				state = SPINNING_FIRST_BEACON;
				spinTimer.start();
				drive.rotateRight(8);
				break;
			}

		break;

		case SPINNING_FIRST_BEACON:
			//rotateRight(8);
			if (spinTimer.isExpired()){
				state = SCANNING_FIRST_BEACON;
				scanTimer.start();
				drive.stop();
				break;
			}

		break;

		case HIT_FIRST_VINYL:
			pos = getBeaconPosition();
			if (analogRead(VINYL) < approxVinyl) {
				state = SPINNING_SECOND_BEACON;
				stopAndReverse(700); //this needs to be adjusted
				numbSpin(9, 400);
				drive.rotateRight(10); //NOTE FASTER SPEED FOR THIS INITIAL SPIN. SHOULD REMOVE ONE STATE
				spinTimer.start();
				break;
			} else if (pos == BEACON_CENTER){
				missedFirstBeaconTimer.start();
				drive.driveForward(10);
			}
			else if (pos == BEACON_LEFT){
				missedFirstBeaconTimer.start();
				drive.drive(9, 10);
			}
			else if (pos == BEACON_RIGHT){
				missedFirstBeaconTimer.start();
				drive.drive(10, 9);
			}
			else if (pos == NO_BEACON) {
				if (!missedFirstBeaconTimer.isExpired()) break;
				stopAndReverse(700);
				numbSpin(9, 400);
				state = SPINNING_SECOND_BEACON;
				drive.rotateRight(10); //NOTE FASTER SPEED FOR THIS INITIAL SPIN. SHOULD REMOVE ONE STATE
				spinTimer.start();
				break;
			}

		break;

		case SPINNING_SECOND_BEACON:
			if (spinTimer.isExpired()){
				state = SCANNING_SECOND_BEACON;
				scanTimer.start();
				drive.stop();
				fieldSideCounter++;
				break;
			}

		break;

		case SCANNING_SECOND_BEACON:
			avg = getAverageBeaconValue();
			if (avg > 60 && avg < 200){ //need to change case
				state = HIT_SECOND_VINYL;
				//state = DEAD;
				//break;
				if (fieldSideCounter > NUM_TURNS){
					fieldSide = LEFT_SIDE;
					drive.drive(6,7);
				}
				else {
					fieldSide = RIGHT_SIDE;
					drive.drive(7,6);
				}
				arcTimer.start();

				break;
			}
			if (scanTimer.isExpired()){
				state = SPINNING_SECOND_BEACON;
				spinTimer.start();
				drive.rotateRight(8);
			}

		break;

		case HIT_SECOND_VINYL:
			//WAIT UNTIL INITIAL ARC COMPLETE
			if (!arcTimer.isExpired()){
				break;
			}
			//IF WE HIT THE VINYL, NEXT STATE
			if (analogRead(VINYL) < approxVinyl){
				
				state = SPINNING_SERVER;
				if (fieldSide == LEFT_SIDE){
					drive.rotateLeft(10);
				}
				if (fieldSide == RIGHT_SIDE){
					drive.rotateRight(8);
				}
				spinTimer.start();
				break;
			}
			//NO VINYL? KEEP GOING, JUST DON'T HIT THE WALL
			wallDist = sonar.ping_in();
			if (wallDist < MIN_DIST && wallDist > 0){
				if (fieldSide == LEFT_SIDE){
					drive.rotateRight(8);
				}
				if (fieldSide == RIGHT_SIDE){
					drive.rotateLeft(8);
				}
				break;
			}
			drive.driveForward(8);

		break;

		case SPINNING_SERVER:
			if (spinTimer.isExpired()){
				state = SCANNING_SERVER;
				drive.stop();
				scanTimer.start();
				break;
			}

		break;

		case SCANNING_SERVER:
			avg = getAverageBeaconValue();
			if (avg > AVG_MINIMUM){
				state = HIT_SERVER;
				drive.driveForward(8);
				break;
			}

			if (scanTimer.isExpired()){
				state = SPINNING_SERVER;
				if (fieldSide == LEFT_SIDE){
					drive.rotateLeft(8);
				}
				else {
					drive.rotateRight(8);
				}
				spinTimer.start();
				break;
			}

		break;

		case HIT_SERVER:
			if (getAverageBeaconValue() > AVG_MINIMUM && sonar.convert_in(sonar.ping_median(7)) < 2 && sonar.ping_in() > 0){
				state = MINING;
				break;
			}

		break;

		case MINING:
			drive.stop();
			mineCoin();
			extendTimer.start();
			if (hitCounter == 7){
				state = DEAD;
			}

		break;

		case DEAD:
			drive.stop();

		break;
	}
}

void mineCoin(){
	miner.extend();
	delay(500);
	miner.retract();
	delay(500);
	hitCounter++;
}

void stopAndReverse(int time) {
	drive.stop();
	delay(500);
	drive.driveBackward(9);
	delay(time);
	drive.stop();
	delay(500);
}

void numbSpin(int speed, int time){
	drive.stop();
	delay(500);
	drive.rotateRight(speed);
	delay(time);
	drive.stop();
	delay(500);
}

int getBeaconPosition() {
	int leftRead = analogRead(LEFT);
	int middleRead = analogRead(MIDDLE);
	int rightRead = analogRead(RIGHT); 

	unsigned int avg = leftRead + rightRead + middleRead;
	avg = avg/3;
 
	if ((leftRead > 400 && middleRead > 400 && rightRead > 400) || (abs(leftRead - rightRead) < 50 && leftRead > 100)) {
		return BEACON_CENTER;
	} else if (middleRead > 200 && middleRead > leftRead) {
		return BEACON_RIGHT;
	} else if (middleRead > 200 && middleRead > rightRead) {
		return BEACON_LEFT;
	} else {
  		return NO_BEACON;
 	}
}

int getAverageBeaconValue() {
	int leftRead = analogRead(LEFT);
	int middleRead = analogRead(MIDDLE);
	int rightRead = analogRead(RIGHT); 

	unsigned int avg = leftRead + rightRead + middleRead;
	avg = avg/3;

	//Serial.print("Average Beacon Value: ");
	//Serial.println(avg);

	return avg;
}