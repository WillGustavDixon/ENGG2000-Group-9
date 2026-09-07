#include <IRremote.hpp> // Using the modern IRremote v4.x syntax

const int NN_IR =  4;
const int NE_IR =  5;
const int EE_IR =  6;
const int SE_IR =  7;
const int SS_IR =  8;
const int SW_IR =  9;
const int WW_IR =  10;
const int NW_IR =  11;

const int SG_IR = 12; // signal IR

volatile bool nnDetected = false;
volatile bool neDetected  = false;
volatile bool eeDetected  = false;
volatile bool seDetected = false;
volatile bool ssDetected = false;
volatile bool swDetected = false;
volatile bool wwDetected  = false;
volatile bool nwDetected  = false;
volatile bool bools[8];

void setup() {
    Serial.begin(115200);
    
    IrSender.begin(SG_IR);

    pinMode(NN_IR, INPUT_PULLUP);
    pinMode(NE_IR, INPUT_PULLUP);
    pinMode(EE_IR, INPUT_PULLUP);
    pinMode(SE_IR, INPUT_PULLUP);
    pinMode(SS_IR, INPUT_PULLUP); 
    pinMode(SW_IR, INPUT_PULLUP); 
    pinMode(WW_IR, INPUT_PULLUP);
    pinMode(NW_IR, INPUT_PULLUP);

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
    

    Serial.println("8 IR SENSOR TEST");
}

ISR(PCINT0_vect) {
    // Pins 8-11
    if (digitalRead(SS_IR) == LOW) ssDetected = true;
    if (digitalRead(SW_IR) == LOW) swDetected = true;
    if (digitalRead(WW_IR) == LOW) wwDetected = true;
    if (digitalRead(NW_IR) == LOW) nwDetected = true;
}
ISR(PCINT2_vect) {
    // Pins 4-7
    if (digitalRead(NN_IR) == LOW) nnDetected = true;
    if (digitalRead(NE_IR) == LOW) neDetected = true;
    if (digitalRead(EE_IR) == LOW) eeDetected = true;
    if (digitalRead(SE_IR) == LOW) seDetected = true;
}

void emitIR() {
    IrSender.enableIROut(38);  // 38 kHz carrier
    IrSender.mark(5000);       // 5 ms of 38 kHz IR
    IrSender.space(1000);       // 1000 us off
}

void getDetectStates() {
    bools[0] = nnDetected;
    bools[1] = neDetected;
    bools[2] = eeDetected;
    bools[3] = seDetected;
    bools[4] = ssDetected;
    bools[5] = swDetected;
    bools[6] = wwDetected;
    bools[7] = nwDetected;
}

void resetDetectStates() {
    nnDetected = false;
    neDetected = false;
    eeDetected = false;
    seDetected = false;
    ssDetected = false;
    swDetected = false;
    wwDetected = false;
    nwDetected = false;
}

void loop() {
    resetDetectStates();
    
    emitIR();
    delay(2); // wait a tiny bit to ensure receivers have detected

    getDetectStates();
    Serial.print(        " NN: ");  Serial.print(bools[0]? "DETECTED" : "-");
    Serial.print("    |    NE: ");  Serial.print(bools[1]? "DETECTED" : "-");
    Serial.print("    |    EE: ");  Serial.print(bools[2]? "DETECTED" : "-");
    Serial.print("    |    SE: ");  Serial.print(bools[3]? "DETECTED" : "-");
    Serial.print("    |    SS: ");  Serial.print(bools[4]? "DETECTED" : "-");
    Serial.print("    |    SW: ");  Serial.print(bools[5]? "DETECTED" : "-");
    Serial.print("    |    WW: ");  Serial.print(bools[6]? "DETECTED" : "-");
    Serial.print("    |    NW: ");  Serial.print(bools[7]? "DETECTED" : "-");
    Serial.println();

    delay(100);
}