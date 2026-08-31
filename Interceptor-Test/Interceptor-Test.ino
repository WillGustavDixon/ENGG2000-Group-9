const int EN_PIN = A0;	// IN1 pin (enable)
const int PH_PIN = A1;	// IN2 pin (phase)
const int SLP_PIN = A2; 	// sleep pin
const int LAS_PIN = 12; // laser pin
const int IRR_PINS[] = {4,6,8,10}; // IR receiver pins
const int ENC_PIN_A = 2; 
const int ENC_PIN_B = 3; 

/* RECEIVER LAYOUT:
                  
         D11  * * * *  D4 (22.5°)
           *          *
      D10 *            * D5 (67.5°)
         *              *   
      D9 *            * D6 (112.5°)
           *          *
           D8 * * * * D7 (157.5°)
*/
const int IRR_PINS_AMT = sizeof(IRR_PINS) / sizeof(IRR_PINS[0]);

const int SPEED = 128;
const unsigned long IR_INTERVAL = 100; // interval to check for pulses in ms
const unsigned long IR_THRESHOLD = 1; // any pulse counts as detection 
const unsigned long IR_COOLDOWN = 1000; // interval after firing laser to wait before detecting again.

int curSpd = SPEED;
bool cd = false; // cooldown
int cdTimer = 0; 

volatile unsigned long irPulseCount = 0;
static unsigned long lastCheck = 0;
volatile unsigned long totalMotorPulses = 0;
int pulseToDeg = 0;

/* INPUT/OUTPUT: N/A
// FUNCTION: Interrupt function that detects when an IR receiver gets something
void detectIR() {
    if(!irDetected && !cd) {
        irPulseCount++;
        //Serial.println("Pulsed!");
    }
}*/

void countPulses() {
    digitalRead(ENC_PIN_B) > 0? totalMotorPulses++ : totalMotorPulses--;
}

void beginCooldown() {
    cd = true;
    cdTimer = IR_COOLDOWN;
}

// INPUT: N/A;    OUTPUT: whether IR signal was found.
// FUNCTION: check all IRR pins for any input, if there is check if pulse frequency meets the threshold and return true if it does
float findIRSignal() {
    float deg = 0; 
    if (millis() - lastCheck >= IR_INTERVAL && !cd) { // if [total ms] - [ms since last check] >= interval of reading IR pins + cooldown if on
        //Serial.println("Time to check...   ");
        lastCheck = millis();
        
        //noInterrupts();
        for(int i = 0; i < IRR_PINS_AMT; i++) { // for each pin, check if it has a signal
            Serial.print(digitalRead(IRR_PINS[i])); Serial.print(", ");
            if(digitalRead(IRR_PINS[i]) == LOW) {
                Serial.print("Found something at ");
                Serial.println(IRR_PINS[i]);
                deg = degToRotate(i);  
                break; // when found, break the loop. 
            }
        }
        Serial.println(" "); 
        //interrupts();
    }
    else if(cd) {
        cdTimer <= 0? cd = false: cdTimer--;
        Serial.println("Cooling down");
    }
    return deg;
}

int getMotorPos() {
    int tMP = map(totalMotorPulses, 0, 700, 0, 360);
    int mPos = abs(tMP % 360); 
    return mPos;
}

// INPUT: the position of the pin that found a signal; OUTPUT: degrees to rotate motor
// FUNCTION: depending on which IR receiver found a signal, determine how many degrees motor must rotate to face that direction (NEEDS HALL ENCODER TO WORK) 
// e.g. mPos = 179, pPos = 1, dest = 67.5: mPos > dest so -1(179 - 67.5) = -111.5. mPos = 40, pPos = 4, dest = 202.5: mPos < dest so 202.5 - 40 = 162.5.
float degToRotate(int pinPos) {
    int mPos = getMotorPos(); float dest = (45*pinPos)+22.5f; float deg; 
    mPos >= dest? deg = -1*(mPos - dest): deg = dest - mPos; // if mPos > dest, deg is negative difference between them, otherwise deg is positive difference
    return deg; // return distance of motor rotation from IR sensor. 

    // WHAT if we have 2 IRRs reporting a signal? since IR emitters can be anywhere around the satellite
}

// INPUT: direction to move motor; OUTPUT: N/A
// FUNCTION: guess
void startMotor(bool dir) {
    analogWrite(EN_PIN, SPEED); // bring motor to speed
    digitalWrite(PH_PIN, dir); // set direction to specified 
    curSpd = SPEED;  
}


// INPUT/OUTPUT: N/A;
// FUNCTION: stop motor, fire laser for 2 seconds, start motor again
void fireLaser() {
    Serial.println("Firing laser...");

    analogWrite(EN_PIN, 0);

    digitalWrite(LAS_PIN, HIGH); // turn on laser
    delay(2000);                 // for 2 seconds
    digitalWrite(LAS_PIN, LOW); // turn off laser
    analogWrite(EN_PIN, SPEED); // begin spinning motor again
    Serial.println("Laser fired, resuming search");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Begin Interceptor Test\n");
    analogWrite(EN_PIN, 0); // make sure motor is not moving at start

    // initialise IR pins
    for(int i = 0; i < IRR_PINS_AMT; i++) pinMode(IRR_PINS[i], INPUT_PULLUP);
	
    pinMode(ENC_PIN_A, INPUT);
    pinMode(ENC_PIN_B, INPUT);
    attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), countPulses, RISING);

    pinMode(EN_PIN, OUTPUT); // set the rest of the pins' modes to output.
    pinMode(PH_PIN, OUTPUT);
    pinMode(SLP_PIN, OUTPUT);
	pinMode(LAS_PIN, OUTPUT);
	digitalWrite(SLP_PIN, HIGH); // wake up the driver

    startMotor(0); // 0 is clockwise, 1 is anti
}

void loop() { 
    float deg = findIRSignal();
    pulseToDeg = getMotorPos();
	if(deg > 0 || deg < 0) { // if an IR signal is detected 
        Serial.print("Found IR Signal at the following degrees, firing laser: "); Serial.println(deg);
        fireLaser();
        beginCooldown();
	}
    //Serial.println(pulseToDeg); 
}
