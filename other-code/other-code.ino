#include <IRremote.hpp>

const int NN_IR =  4;
const int NE_IR =  5;
const int EE_IR =  6;
const int SE_IR =  7;
const int SS_IR =  8;
const int SW_IR =  9;
const int WW_IR =  10;
const int NW_IR =  11;

const int SG_IR = 12; // signal IR

volatile int nnDetected = 0;
volatile int neDetected  = 0;
volatile int eeDetected  = 0;
volatile int seDetected = 0;
volatile int ssDetected = 0;
volatile int swDetected = 0;
volatile int wwDetected  = 0;
volatile int nwDetected  = 0;
volatile int hits[8];

static unsigned long lastCheck = 0;

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
    if (digitalRead(SS_IR) == LOW) ssDetected++;
    if (digitalRead(SW_IR) == LOW) swDetected++;
    if (digitalRead(WW_IR) == LOW) wwDetected++;
    if (digitalRead(NW_IR) == LOW) nwDetected++;
}
ISR(PCINT2_vect) {
    // Pins 4-7
    if (digitalRead(NN_IR) == LOW) nnDetected++;
    if (digitalRead(NE_IR) == LOW) neDetected++;
    if (digitalRead(EE_IR) == LOW) eeDetected++;
    if (digitalRead(SE_IR) == LOW) seDetected++;
}

void emitIR() {
    IrSender.enableIROut(38);  // 38 kHz carrier
    IrSender.mark(5000);       // 5 ms of 38 kHz IR
    IrSender.space(1000);       // 1000 us off
}

void getDetectStates() {
    hits[0] = nnDetected;
    hits[1] = neDetected;
    hits[2] = eeDetected;
    hits[3] = seDetected;
    hits[4] = ssDetected;
    hits[5] = swDetected;
    hits[6] = wwDetected;
    hits[7] = nwDetected;
}

void resetDetectStates() {
    nnDetected = 0;
    neDetected = 0;
    eeDetected = 0;
    seDetected = 0;
    ssDetected = 0;
    swDetected = 0;
    wwDetected = 0;
    nwDetected = 0;
}

void loop() {
    if (millis() - lastCheck >= 100) {
        Serial.print(millis()); Serial.print(":    ");
        lastCheck = millis();

        noInterrupts();
        
        getDetectStates();
        resetDetectStates();
        
        interrupts();
        Serial.print(        " NN: ");  Serial.print(hits[0]);
        Serial.print("    |    NE: ");  Serial.print(hits[1]);
        Serial.print("    |    EE: ");  Serial.print(hits[2]);
        Serial.print("    |    SE: ");  Serial.print(hits[3]);
        Serial.print("    |    SS: ");  Serial.print(hits[4]);
        Serial.print("    |    SW: ");  Serial.print(hits[5]);
        Serial.print("    |    WW: ");  Serial.print(hits[6]);
        Serial.print("    |    NW: ");  Serial.print(hits[7]);
        Serial.println();
    }
}