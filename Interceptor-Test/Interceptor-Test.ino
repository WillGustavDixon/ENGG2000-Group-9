#include <IRremote.hpp> // Using the modern IRremote v4.x syntax

/* RECEIVER LAYOUT:
                  
         D11  * * * *  D4 (22.5°)
           *          *
      D10 *            * D5 (67.5°)
         *              *   
      D9 *            * D6 (112.5°)
           *          *
           D8 * * * * D7 (157.5°)
*/

const int EN_PIN = A0;	// IN1 pin (enable)
const int PH_PIN = A1;	// IN2 pin (phase)
const int SLP_PIN = A2; // sleep pin, wakes motor driver.
const int ENC_PIN_A = 2;  // encoder pin A 
const int ENC_PIN_B = 3;  // encoder pin B

const int IRR_PINS[] = {4,5,6,7,8,9,10,11}; // IR receiver pins
const int IRR_PINS_AMT = sizeof(IRR_PINS) / sizeof(IRR_PINS[0]);
volatile int IRR_HITS[IRR_PINS_AMT];
String RECEIVER_DIRS[] = {"NN", "NE", "EE", "SE", "SS", "SW", "WW", "NW"};

const int LAS_PIN = 12; // laser pin

const float CPD = 1.9444; 
const int SPR = 700;
const int SPEED = 128;
int curSpd = SPEED; // R/W variable that is altered instead of the const SPEED.

static unsigned long lastCheck = 0;
volatile unsigned long totalMotorPulses = 0;
bool foundIR = false;
String foundRec = "";


void countPulses() {
    digitalRead(ENC_PIN_B) > 0? totalMotorPulses++ : totalMotorPulses--;
    //Serial.println(getMotorPos());
}

float getMotorPos() {
    return (totalMotorPulses % 360) / CPD;
}

// INPUT: the position of the pin that found a signal; OUTPUT: degrees to rotate motor
// FUNCTION: depending on which IR receiver found a signal, determine how many degrees motor must rotate to face that direction (NEEDS HALL ENCODER TO WORK) 
// e.g. mPos = 179, pPos = 1, dest = 67.5: mPos > dest so -1(179 - 67.5) = -111.5. mPos = 40, pPos = 4, dest = 202.5: mPos < dest so 202.5 - 40 = 162.5.
float stepsToRotate(int pinPos) {
    int motorPos = getMotorPos(); 
    float dest = (45*pinPos); 
    float deg; 
    motorPos >= dest? deg = -1*(motorPos - dest) : deg = dest - motorPos; // if mPos > dest, deg is negative difference between them, otherwise deg is positive difference
    return (deg); // return distance of motor rotation from IR sensor. 
}

float checkIR() {
    for(int i = 0; i < IRR_PINS_AMT; i++) { 
        if(IRR_HITS[i] > 5) {
            foundIR = true;
            foundRec = RECEIVER_DIRS[i];
            return stepsToRotate(i);
        }
    }
}

// INPUT: direction to move motor; OUTPUT: N/A
// FUNCTION: guess
void startMotor(bool dir) {
    analogWrite(EN_PIN, SPEED); // bring motor to speed
    digitalWrite(PH_PIN, dir); // set direction to specified 
    curSpd = SPEED;  
}

long setShortestPathTarget(float targ) {
  // Convert degrees to absolute step position (0 to 699)
  long targMod = round(targ * CPD) % SPR;
  if (targMod < 0) targMod += SPR; // Handle negative degree inputs
  
  // Find where motor is currently (0 to 699)
  long curMod = totalMotorPulses % SPR;
  if (curMod < 0) curMod += SPR;
  
  // Calculate the shortest change in steps (-350 to +350)
  long stepDiff = targMod - curMod;
  
  if (stepDiff > (SPR/2)) {
    stepDiff -= SPR; // Faster to go backward
  } 
  else if (stepDiff < -(SPR/2)) {
    stepDiff += SPR; // Faster to go forward
  }
  return (totalMotorPulses + stepDiff);
}

// INPUT: target rotation relative to current position
// FUNCTION: activate motor for short time until at target
void rotateToPos(float targ) {
    long error = targ - totalMotorPulses;
    int initDeg = getMotorPos();
    int curDeg = initDeg;
    bool anti;
    int counter = 0;
    targ < 0? anti = true : anti = false; // if targ is negative, we move anticlockwise
    startMotor(anti);
    while((anti? curDeg > targ : targ > curDeg) && counter < 360) {
        curDeg = getMotorPos(); Serial.println(curDeg);
        //counter = abs(curDeg - initDeg);
    }
    analogWrite(EN_PIN, 0);
}

// INPUT/OUTPUT: N/A;
// FUNCTION: stop motor, fire laser for 2 seconds, start motor again
void fireLaser() {
    Serial.println("Firing laser...");

    digitalWrite(LAS_PIN, HIGH); // turn on laser
    delay(2000);                 // for 2 seconds
    digitalWrite(LAS_PIN, LOW); // turn off laser

    Serial.println("Laser fired, resuming search");
}

void resetHits() {
    for(int i = 0; i < IRR_PINS_AMT; i++) {
        IRR_HITS[i] = 0;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("| Begin Interceptor Test |\n");

    Serial.print("IR Receivers ");
    for(int i = 0; i < IRR_PINS_AMT; i++) {
        pinMode(IRR_PINS[i], INPUT_PULLUP);
        IRR_HITS[i] = 0;
        Serial.print(IRR_PINS[i]);
        Serial.print(", ");
    }
    Serial.println("initialised.");

    // Enable pin-change interrupts on ranges 8-13 & 0-7
    PCICR |= (1 << PCIE0);  
    PCICR |= (1 << PCIE2);  
    // Then enable for each used pin:
    PCMSK2 |= (1 << PCINT20); // Pin 4  (NN)
    PCMSK2 |= (1 << PCINT21); // Pin 5  (NE)
    PCMSK2 |= (1 << PCINT22); // Pin 6  (EE)
    PCMSK2 |= (1 << PCINT23); // Pin 7  (SE)
    PCMSK0 |= (1 << PCINT0);  // Pin 8  (SS)
    PCMSK0 |= (1 << PCINT1);  // Pin 9  (SW)
    PCMSK0 |= (1 << PCINT2);  // Pin 10 (WW)
    PCMSK0 |= (1 << PCINT3);  // Pin 11 (NW)
    	
    pinMode(ENC_PIN_A, INPUT); // set pins to read motor encoder as input
    pinMode(ENC_PIN_B, INPUT);
    attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), countPulses, RISING);
    Serial.println("Motor encoder channels initialised.");

    pinMode(EN_PIN, OUTPUT); // set the rest of the pins' modes to output.
    pinMode(PH_PIN, OUTPUT);
    pinMode(SLP_PIN, OUTPUT);
    Serial.println("Motor driver initialised.");

	pinMode(LAS_PIN, OUTPUT);
	
    digitalWrite(SLP_PIN, HIGH); // wake up the driver
    analogWrite(EN_PIN, 0); // make sure motor is not moving at start
    delay(1000);
}

ISR(PCINT0_vect) {
    // Pins 8-11
    if (digitalRead(IRR_PINS[4]) == LOW) IRR_HITS[4]++;
    if (digitalRead(IRR_PINS[5]) == LOW) IRR_HITS[5]++;
    if (digitalRead(IRR_PINS[6]) == LOW) IRR_HITS[6]++;
    if (digitalRead(IRR_PINS[7]) == LOW) IRR_HITS[7]++;
}
ISR(PCINT2_vect) {
    if (digitalRead(IRR_PINS[0]) == LOW) IRR_HITS[0]++;
    if (digitalRead(IRR_PINS[1]) == LOW) IRR_HITS[1]++;
    if (digitalRead(IRR_PINS[2]) == LOW) IRR_HITS[2]++;
    if (digitalRead(IRR_PINS[3]) == LOW) IRR_HITS[3]++;
}

void loop() { 
    if (millis() - lastCheck >= 100) {
        lastCheck = millis();
        //delay(2); // wait a tiny bit to ensure receivers have detected

        float steps = checkIR();
        if(foundIR) { // if an IR signal is detected 
            float deg = getMotorPos();
            Serial.print("Found IR Signal at receiver  " + foundRec + ",  move to  "); Serial.print(steps); Serial.print("  from  "); Serial.println(deg);
            rotateToPos(steps);
            fireLaser();
            delay(1000); // buffer period for testing
            foundIR = false;
        }

        noInterrupts();
        resetHits();
        interrupts();
    }
}
