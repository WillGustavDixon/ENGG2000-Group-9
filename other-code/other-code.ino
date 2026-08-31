#define IR_SEND_PIN 12  // Connect XC4426 'S' to this pin
#define IR_RECEIVE_PIN 4  // Connect XC4427 'S' to this pin

#include <IRremote.hpp> // Using the modern IRremote v4.x syntax

void setup() {
  Serial.begin(115200);
  
  // Initialize the sender on your custom pin
  IrSender.begin(IR_SEND_PIN);
  
  // Initialize the receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.println("IR Transmitter and Receiver Initialized.");
}

void loop() {
  // 1. Transmit a custom 16-bit pulse payload at 38kHz
  uint16_t address = 0x01;
  uint8_t command = 0x34;
  
  Serial.println("Sending IR pulse...");
  IrSender.sendNEC(address, command, 0); 
  
  delay(100); // Small delay to allow transmission to clear
  
  // 2. Check if the receiver module picked it up
  if (IrReceiver.decode()) {
    Serial.println("Pulse received successfully!");
    Serial.print("Data: 0x");
    Serial.println(IrReceiver.decodedIRData.command, HEX);
    
    IrReceiver.resume(); // Enable receiving the next value
  }
  
  delay(1000); // Wait 1 second before pulsing again
}