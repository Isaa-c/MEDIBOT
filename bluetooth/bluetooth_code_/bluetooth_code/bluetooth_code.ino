#include "AFMotor.h"

// Define motor constants for readability
const int MOTOR_1 = 1; 
const int MOTOR_2 = 2; 
const int MOTOR_3 = 3; 
const int MOTOR_4 = 4; 

// Initialize DC motors with their respective frequencies
AF_DCMotor motor1(MOTOR_1, MOTOR12_64KHZ); 
AF_DCMotor motor2(MOTOR_2, MOTOR12_64KHZ); 
AF_DCMotor motor3(MOTOR_3, MOTOR12_64KHZ); 
AF_DCMotor motor4(MOTOR_4, MOTOR12_64KHZ); 

// Declare variables to store the current state and speed
int state = 0; // Initialize to a default value
int Speed = 130; // Default motor speed

void setup() {
    // Set initial speed for all motors
    motor1.setSpeed(Speed);  
    motor2.setSpeed(Speed);
    motor3.setSpeed(Speed);
    motor4.setSpeed(Speed);  
    
    // Initialize Serial communication for control commands
    Serial.begin(9600);
    delay(500); 
}

void loop() {   
    // Check if there's any serial input available
    if (Serial.available() > 0) {   
        state = Serial.read();      
        // Update speed if the received state is a speed value
        if (state > 10) {
            Speed = state;
        }      
    }
           
    // Set the speed for all motors
    motor1.setSpeed(Speed);          // Set the motor speed to 0-255
    motor2.setSpeed(Speed);
    motor3.setSpeed(Speed);
    motor4.setSpeed(Speed);

    //Control Command
 
         if (state == 1) { forward(); }  // Move forward when state is '1'
    else if (state == 2) { backward(); } // Move backward when state is '2'
    else if (state == 3) { turnLeft(); } // Turn left when state is '3'
    else if (state == 4) { turnRight(); } // Turn right when state is '4'
    else if (state == 5) { Stop(); }     // Stop the motor when state is '5'

    delay(80);    
}

// Function to move all motors forward
void forward() {
    motor1.run(FORWARD); // Motor 1 forward
    motor2.run(FORWARD); 
    motor3.run(FORWARD); 
    motor4.run(FORWARD);
}

// Function to move all motors backward
void backward() {
    motor1.run(BACKWARD); // Motor 1 backward
    motor2.run(BACKWARD);
    motor3.run(BACKWARD); 
    motor4.run(BACKWARD); 
}

// Function to turn right
void turnRight() {
    motor1.run(FORWARD); // Motor 1 forward
    motor2.run(FORWARD); 
    motor3.run(BACKWARD); // Motor 3 backward
    motor4.run(BACKWARD);
}

// Function to turn left
void turnLeft() {
    motor1.run(BACKWARD); // Motor 1 backward
    motor2.run(BACKWARD);
    motor3.run(FORWARD);  // Motor 3 forward
    motor4.run(FORWARD); 
}

// Function to stop all motors
void Stop() {
    motor1.run(RELEASE); // Stop motor 1
    motor2.run(RELEASE);
    motor3.run(RELEASE);
    motor4.run(RELEASE);
}
