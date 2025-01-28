#include <Wire.h>
#include <AFMotor.h>

AF_Stepper Stepper1(20, 2); // Stepper Motor connected to Motor Shield
bool isOpen = false;        // Tracks the motor state: false = closed, true = open

void setup() {
  Wire.begin(8); // Start I2C as Slave with address 8
  Wire.onReceive(receiveCommand); // Register the I2C receive event
  Serial.begin(9600);
  Stepper1.setSpeed(255); // Set stepper motor speed
  Serial.println("System initialized. Gate is initially closed.");
}

void loop() {
  // The slave loop remains empty as actions happen on I2C commands
}

void receiveCommand(int bytes) {
  while (Wire.available()) {
    char command = Wire.read(); // Read the command
    Serial.print("Command received: ");
    Serial.println(command);

    if (command == 'O') {
      openGate();
    } else if (command == 'C') {
      closeGate();
    }
  }
}

void openGate() {
  if (isOpen) {
    Serial.println("Gate is already open. No action needed.");
    return;
  }
  
  Serial.println("Opening gate...");
  Stepper1.step(250, FORWARD, MICROSTEP); // Adjust steps if needed
  isOpen = true; // Update state
  Serial.println("Gate is now open.");
}

void closeGate() {
  if (!isOpen) {
    Serial.println("Gate is already closed. No action needed.");
    return;
  }

  Serial.println("Closing gate...");
  Stepper1.step(250, BACKWARD, MICROSTEP); // Adjust steps if needed
  isOpen = false; // Update state
  Serial.println("Gate is now closed.");
}
