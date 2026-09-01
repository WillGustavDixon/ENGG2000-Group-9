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
    PCMSK2 |= (1 << PCINT20); // Pin 4
    PCMSK2 |= (1 << PCINT22); // Pin 6
    

    Serial.println("4 IR SENSOR TEST");
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

void loop() {
    // reset all detection states
    nDetected = false; eDetected = false; sDetected = false; wDetected = false;
    
    emitIR();
    delay(2); // wait a tiny bit to ensure receivers have detected

    Serial.print(" NORTH: ");         Serial.print(nDetected? "DETECTED" : "-");
    Serial.print("    |    EAST: ");  Serial.print(eDetected? "DETECTED" : "-");
    Serial.print("    |    SOUTH: "); Serial.print(sDetected? "DETECTED" : "-");
    Serial.print("    |    WEST: ");  Serial.print(wDetected? "DETECTED" : "-");
    Serial.println();

    delay(100);
}