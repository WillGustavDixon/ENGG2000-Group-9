// * MOTOR PINS * //
const int ENC_PIN_A = 2;  // encoder pin A
const int ENC_PIN_B = 3;  // encoder pin B
const int PH_PIN = 4;    // IN2 pin (phase)
const int EN_PIN = 5;    // IN1 pin (enable)
const int SLP_PIN = 6;   // sleep pin, wakes motor driver.

const int LAS_PIN = 7;  // laser pin

// * IR RECEIVER PINS * // 
String RECEIVER_DIRS[] = { "NN", "NE", "EE", "SE", "SS", "SW", "WW", "NW" }; // which direction is each receiver facing? (NN is where the laser is pointing)
const int IRR_PINS[]   = {  12,   11,   10,    9,    8,   A1,   A0,   A2  }; 


const int IRR_PINS_AMT = sizeof(IRR_PINS) / sizeof(IRR_PINS[0]); // how many IR pins do we have? should be 8
volatile int IRR_HITS[IRR_PINS_AMT]; // for tracking how many IR hits each receiver gets every interval

const long SPR = 700; // Steps per revolution (for this motor, 700)
const float CPD = SPR/360.0;  // Counts per degree (should be 1.9444)

const int MAX_SPEED = 128;
const int MIN_SPEED = 16;
int curSpd = MAX_SPEED;  // R/W variable that is altered instead of the constant values.

const  unsigned long INTERVAL = 100; // how frequently IR hit counts should be checked (ms)
static unsigned long lastCheck = 0; // time (ms) since IR hit counts were checked

volatile long motorStepCount = 0; // how much has the motor moved in steps?
bool foundIR = false; // true when a receiver has found a suitably strong signal
String foundName = ""; // direction of receiver that found the signal (from RECEIVER_DIRS[]) 

const bool PRINT_TELEMETRY = true;

float stepsToDeg(long s) { return (s/CPD);}
long degToSteps(float d) { return (d*CPD);}

void countPulses() {
  digitalRead(ENC_PIN_B) > 0 ? motorStepCount++ : motorStepCount--;
  Serial.print("  Counted!  ");
}

long getMotorPulses() {
    return (motorStepCount % 700);
}

float getMotorDeg() {
    return getMotorPulses() / CPD;
}

// INPUT: the position of the pin that found a signal; OUTPUT: steps motor must rotate to get to signal direction.
// FUNCTION: depending on which IR receiver found a signal, determine how many degrees motor must rotate to face that direction (NEEDS HALL ENCODER TO WORK)
long stepsToRotate(int pinPos) {
    int motorPos = getMotorPulses();
    float dest = (87.5 * pinPos);
    long steps = round(dest - motorPos); // steps = (87.5x - M)

    // if mPos > dest, deg is negative difference between them, otherwise deg is positive difference
    if(steps > 350.0) {
        steps -= 700.0;
    }
    if(steps < -350.0) {
        steps += 700.0;
    }
    
    return steps;   // return distance of motor rotation from IR sensor.
}

long checkIR() {
    int maxHits = 5; // need a minimum of 5 IR hits otherwise its considered noise  
    int irrIndex = 0;
    for (int i = 0; i < IRR_PINS_AMT; i++) {
        if (IRR_HITS[i] > maxHits) {
            foundIR = true;
            irrIndex = i;
            foundName = RECEIVER_DIRS[i];
        }
    }
    return stepsToRotate(irrIndex);
}

// INPUT: direction to move motor; OUTPUT: N/A
// FUNCTION: FALSE is clockwise, TRUE is anticlockwise
void startMotor(bool dir) {
  analogWrite(EN_PIN, MAX_SPEED);  // bring motor to speed
  digitalWrite(PH_PIN, dir);   // set direction to specified
  curSpd = MAX_SPEED;
}

// INPUT: target rotation relative to current position
// FUNCTION: activate motor for short time until at target
void rotateToPos(long steps) {
    long curPos = getMotorPulses();
    long targ = constrain(curPos + steps, -699, 699); // make sure targ is between 0 and 700
    long error = steps;
    float kP = 0.5;
    startMotor(steps < 0);
    Serial.print(" to "); Serial.print(targ); Serial.print(" ("); Serial.print(curPos + steps); Serial.println(")");  
    while(curPos != targ) {
        curPos = getMotorPulses();
        error = targ - curPos;
        Serial.print(curPos); Serial.print(",  "); Serial.println(error);
        curSpd = constrain(abs(error) * kP, MIN_SPEED, MAX_SPEED);
        analogWrite(EN_PIN, curSpd);
    }
    analogWrite(EN_PIN, 0);  // bring motor to stop
}

// INPUT/OUTPUT: N/A;
// FUNCTION: stop motor, fire laser for 2 seconds, start motor again
void fireLaser() {
  Serial.println("Firing laser...");

  digitalWrite(LAS_PIN, HIGH);  // turn on laser
  delay(2000);                  // for 2 seconds
  digitalWrite(LAS_PIN, LOW);   // turn off laser

  Serial.println("Laser fired, resuming search");
}

void resetHits() {
  for (int i = 0; i < IRR_PINS_AMT; i++) {
    IRR_HITS[i] = 0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("| Begin Interceptor Test |\n");

  Serial.print("IR Receivers ");
  for (int i = 0; i < IRR_PINS_AMT; i++) {
    pinMode(IRR_PINS[i], INPUT_PULLUP);
    IRR_HITS[i] = 0;
    Serial.print(IRR_PINS[i]);
    Serial.print(", ");
  }
  Serial.println("initialised.");

  // Enable pin-change interrupts on ranges 8-13, A0-A5
  PCICR |= (1 << PCIE0); 
  PCICR |= (1 << PCIE1); 
  // Then enable for each used pin:
  PCMSK0 |= (1 << PCINT0);  // Pin  8  (NN)
  PCMSK0 |= (1 << PCINT1);  // Pin  9  (NN)
  PCMSK0 |= (1 << PCINT2);  // Pin 10  (NN)
  PCMSK0 |= (1 << PCINT3);  // Pin 11  (NN)
  PCMSK0 |= (1 << PCINT4);  // Pin 12  (NN)
  //PCMSK0 |= (1 << PCINT5);  // Pin 13  (NN)
  PCMSK1 |= (1 << PCINT8);  // Pin A0  (NN)
  PCMSK1 |= (1 << PCINT9);  // Pin A1  (NE)
  PCMSK1 |= (1 << PCINT10);  // Pin A1  (NE)

  pinMode(ENC_PIN_A, INPUT);  // set pins to read motor encoder as input
  pinMode(ENC_PIN_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), countPulses, RISING);
  Serial.println("Motor encoder channels initialised.");

  pinMode(EN_PIN, OUTPUT);  // set the rest of the pins' modes to output.
  pinMode(PH_PIN, OUTPUT);
  pinMode(SLP_PIN, OUTPUT);
  Serial.println("Motor driver initialised.");

  pinMode(LAS_PIN, OUTPUT);

  digitalWrite(SLP_PIN, HIGH);  // wake up the driver
  analogWrite(EN_PIN, 0);       // make sure motor is not moving at start
  delay(1000);
}

ISR(PCINT0_vect) {
  // Pins 8-13
  if (digitalRead(IRR_PINS[0]) == LOW) IRR_HITS[0]++;
  if (digitalRead(IRR_PINS[1]) == LOW) IRR_HITS[1]++;
  if (digitalRead(IRR_PINS[2]) == LOW) IRR_HITS[2]++;
  if (digitalRead(IRR_PINS[3]) == LOW) IRR_HITS[3]++;
  if (digitalRead(IRR_PINS[4]) == LOW) IRR_HITS[4]++;
  
}
ISR(PCINT1_vect) {
  // Pins A0, A1
  if (digitalRead(IRR_PINS[5]) == LOW) IRR_HITS[5]++;
  if (digitalRead(IRR_PINS[6]) == LOW) IRR_HITS[6]++;
  if (digitalRead(IRR_PINS[7]) == LOW) IRR_HITS[7]++;
}

void loop() {
  if (millis() - lastCheck >= INTERVAL) {
    lastCheck = millis();
    //delay(2); // wait a tiny bit to ensure receivers have detected

    long targSteps = checkIR();
    if(PRINT_TELEMETRY) IrInfo();
    if (foundIR) {  // if an IR signal is detected
      long curSteps = getMotorPulses();
      Serial.print("Found IR Signal at receiver  " + foundName + ",  move "); Serial.print(targSteps); Serial.print(" steps from "); Serial.print(curSteps);
      rotateToPos(targSteps);
      fireLaser();
      Serial.print("Current motor position: "); Serial.println(getMotorDeg()); 
      delay(1000);  // buffer period for testing
      foundIR = false;
    }

    noInterrupts();
    resetHits();
    interrupts();
  }
}

void IrInfo() {
    Serial.print(        " NN: ");  Serial.print(IRR_HITS[0]);
    Serial.print("    |    NE: ");  Serial.print(IRR_HITS[1]);
    Serial.print("    |    EE: ");  Serial.print(IRR_HITS[2]);
    Serial.print("    |    SE: ");  Serial.print(IRR_HITS[3]);
    Serial.print("    |    SS: ");  Serial.print(IRR_HITS[4]);
    Serial.print("    |    SW: ");  Serial.print(IRR_HITS[5]);
    Serial.print("    |    WW: ");  Serial.print(IRR_HITS[6]);
    Serial.print("    |    NW: ");  Serial.print(IRR_HITS[7]);
    Serial.println();
}
