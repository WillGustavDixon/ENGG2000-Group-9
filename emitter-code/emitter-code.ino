#include <IRremote.hpp>

const int SG_IR = 12;

void setup() {
    Serial.begin(115200);
    IrSender.begin(SG_IR);
    Serial.println("Emitter on.");
}

void loop() {
    IrSender.enableIROut(38);  // 38 kHz carrier
    IrSender.mark(600);       // 0.6 ms of 38 kHz IR
    IrSender.space(10000);       // 10 ms off
    delay(50);
}
