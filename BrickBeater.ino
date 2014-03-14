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
#define SCANNING_EXCHANGE 11
#define SPINNING_EXCHANGE 12
#define HIT_EXCHANGE_VINYL 13
#define DUMP_COINS 14
#define DEAD 15

//PINS
#define LEFT A5
#define MIDDLE A6
#define RIGHT A7

#define MIDDLE_BACK A3

//BEACON READ VALUES
#define BEACON_CENTER 0
#define BEACON_LEFT 1
#define BEACON_RIGHT 2
#define NO_BEACON 3
#define AVG_MINIMUM 140

#define VINYL A0
#define MIN_DIST 15

#define NUM_TURNS 5
#define LEFT_SIDE 0
#define RIGHT_SIDE 1

#define BASE_SPEED 200

Timer spinTimer(250);
Timer scanTimer(600);
Timer arcTimer(350);
Timer missedFirstBeaconTimer(2000);
Timer ignoreVinylTimer(400);
Timer shortIgnoreVinyl(200);
Timer longScanTimer(800);
Timer foundServerTimer(5000);

NewPing sonar(5, 6, 100);
//NewPing backSonar(8, 10, 100);

Drive drive(210, 13, 12, 11, 3);

Servo miner;
Servo rightMiner;
Servo dumper;


int state = SCANNING_FIRST_BEACON;
int fieldSide = -1;
int fieldSideCounter = 0;
int approxVinyl = -1;
int buttonPushedCount = 0;

int hitCounter = 0;

void setup() {
	Serial.begin(9600);

	miner.attach(9);
	miner.write(110);
	rightMiner.attach(2);
	rightMiner.write(20);

	dumper.attach(7);
	dumper.write(120);

	drive.init(LOW, HIGH);
	scanTimer.start();
	pinMode(A0, INPUT);
	pinMode(LEFT, INPUT);
	pinMode(MIDDLE, INPUT);
	pinMode(RIGHT, INPUT);
	approxVinyl = analogRead(VINYL) - 200;
	//drive.offset(1);
}


void loop() {

	int avg = 0;
	int pos = NO_BEACON;
	int wallDist = 0;

	switch (state){


		case SCANNING_FIRST_BEACON:
			//drive.stop();
			
			avg = getAverageBeaconValue();
			wallDist = sonar.ping_in();
			if (avg > 100 && wallDist > 0 && wallDist < 30){
				state = HIT_FIRST_VINYL;
				ignoreVinylTimer.start();
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
			if (ignoreVinylTimer.isExpired() && analogRead(VINYL) < approxVinyl) {
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
				if (!missedFirstBeaconTimer.isExpired()) {
					drive. driveForward(9);
					break;
				}
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
			wallDist = sonar.ping_in();
			if (avg > 60 && avg < 200 && wallDist == 0){ //need to change case
				state = HIT_SECOND_VINYL;
				ignoreVinylTimer.start();
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
			if (ignoreVinylTimer.isExpired() && analogRead(VINYL) < approxVinyl){
				
				state = SPINNING_SERVER;
				if (fieldSide == LEFT_SIDE){
					drive.rotateLeft(8);
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
					//drive.rotateRight(10);
					drive.stop();
					delay(500);
					drive.rotateRight(9);
					delay(450);
					drive.stop();
					delay(500);
				}
				if (fieldSide == RIGHT_SIDE){
					drive.rotateLeft(7);
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
				foundServerTimer.start();
				state = HIT_SERVER;
				drive.driveForward(8);
				break;
			}

			if (scanTimer.isExpired()){
				state = SPINNING_SERVER;
				if (fieldSide == LEFT_SIDE){
					drive.rotateLeft(7);
				}
				else {
					drive.rotateRight(7);
				}
				spinTimer.start();
				break;
			}

		break;

		case HIT_SERVER:
			if (getAverageBeaconValue() > AVG_MINIMUM && sonar.convert_in(sonar.ping_median(7)) <= 2 && sonar.ping_in() > 0){
				state = MINING;
				break;
			} else if (foundServerTimer.isExpired()) {
				state = MINING;
				break;
			}

		break;

		case MINING:
			if (buttonPushedCount > 7) {
				stopAndReverse(1300);
				state = SCANNING_EXCHANGE;
				numbSpin(8, 600);
				pullBackMiners();
				break;
			}
			drive.stop();
			mineCoin();

		break;

		case SCANNING_EXCHANGE:
			avg = getAverageBeaconValue();
			wallDist = sonar.ping_in();
			if (avg > 250 && avg < 500  && wallDist > 0 && wallDist < 23){
				state = HIT_EXCHANGE_VINYL; //MOVE TO EXCHANGE SET
				shortIgnoreVinyl.start();
				missedFirstBeaconTimer.start();
				break;
			}
			if (longScanTimer.isExpired()){
				state = SPINNING_EXCHANGE;
				spinTimer.start();
				drive.rotateRight(8);
				break;
			}
		break;

		case SPINNING_EXCHANGE:
			if (spinTimer.isExpired()){
				state = SCANNING_EXCHANGE;
				longScanTimer.start();
				drive.stop();
				break;
			}

		break;

		case HIT_EXCHANGE_VINYL:
			pos = getBeaconPosition();
			wallDist = sonar.convert_in(sonar.ping_median(7));
			if (shortIgnoreVinyl.isExpired() && wallDist <= 3 && wallDist > 0) {

				state = DUMP_COINS;
				break;
			} else if (pos == BEACON_CENTER){
				missedFirstBeaconTimer.start();
				drive.driveForward(9);
			}
			else if (pos == BEACON_LEFT){
				missedFirstBeaconTimer.start();
				drive.driveForward(9);
			}
			else if (pos == BEACON_RIGHT){
				missedFirstBeaconTimer.start();
				drive.driveForward(9);
			}
			else if (pos == NO_BEACON) {
				if (!missedFirstBeaconTimer.isExpired()) {
					drive.driveForward(9);
					break;
				}
				stopAndReverse(400);
				state = SCANNING_EXCHANGE;
				drive.rotateRight(9); //NOTE FASTER SPEED FOR THIS INITIAL SPIN. SHOULD REMOVE ONE STATE
				spinTimer.start();
				break;
			}
		break;

		case DUMP_COINS:
			dumpCoins();
			state = DEAD;


		break;

		case DEAD:
			drive.stop();

		break;
	}
}


void pullBackMiners() {
	miner.write(110);
	rightMiner.write(20);
}

void dumpCoins() {
	dumper.write(120); //STARTING POSITION OF DUMPER MECH
	delay(500);
	dumper.write(150);
	delay(2000);
	dumper.write(120);
}

void mineCoin(){
	miner.write(50);
	rightMiner.write(85);
	delay(400);
	miner.write(70);
	rightMiner.write(60);
	delay(400);
	buttonPushedCount++;
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