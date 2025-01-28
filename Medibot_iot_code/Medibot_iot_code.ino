#include <Wire.h>
#include "thingProperties.h"
#include <DHT.h>
#include <math.h>

// Define DHT pins and types  
#define DHT1_PIN 4   // DHT11 sensor 1 connected to digital pin 4
#define DHT2_PIN 5   // DHT11 sensor 2 connected to digital pin 5
#define DHT_TYPE DHT11

DHT dht1(DHT1_PIN, DHT_TYPE);
DHT dht2(DHT2_PIN, DHT_TYPE);

// Define MQ-7 pin and constants
#define MQ7_PIN A0
const float RL = 10.0;  // Load resistance in kOhms
const float RO = 10.0;  // Sensor resistance in clean air (calibrated)

// Define Sound Sensor pin
#define SOUND_SENSOR_PIN A2 // Analog pin connected to the sound sensor
#define REFERENCE_VOLTAGE 0.01 // Reference voltage for quiet environment (calibrate)
const float SOUND_THRESHOLD_DB = 65.0; // Threshold level in dB (adjust as needed)

// Define Flame Sensor pin
#define FLAME_SENSOR_PIN 6 // Digital pin for flame sensor

// Define IR Sensor and Relay for Sanitizer Dispenser
#define IRSENSOR_PIN 10      // IR sensor connected to pin 10
#define RELAY_MODULE_PIN 12  // Relay module connected to pin 12
#define GREEN_LED_PIN 9      // Green LED connected to pin 9

// Define Relays for Cloud Variables
#define DISINFECTION_LIGHT_PIN 7 // Relay for disinfection light
#define FRONT_LIGHT_PIN 8        // Relay for front light
#define MEDIBOT_LIGHT_PIN 11     // Relay for medibot light

// Thresholds for harmful levels
const float PPM_THRESHOLD = 35.0;  // Harmful gas threshold in ppm
const int AQI_THRESHOLD = 100;     // Harmful gas threshold in AQI

// Global Variables
int sensor_value; // Variable to hold IR sensor state

// Function to calculate Rs/Ro
float calculateRsRo(int analogValue) {
  float sensorVoltage = analogValue * (5.0 / 1023.0); // Convert analog value to voltage
  float Rs = RL * (5.0 - sensorVoltage) / sensorVoltage; // Calculate sensor resistance (Rs)
  return Rs / RO; // Return the ratio Rs/Ro
}

// Function to convert Rs/Ro ratio to CO ppm
float calculatePPM(float rsRoRatio) {
  float a = -0.45; // Slope of the logarithmic curve
  float b = 1.2;   // Intercept of the logarithmic curve
  return pow(10, (a * log10(rsRoRatio) + b)); // Calculate ppm using logarithmic scale
}

// Function to map CO ppm to AQI
float calculateAQI(float ppm) {
  if (ppm <= 4.4) return map(ppm, 0.0, 4.4, 0, 50);          // Good
  else if (ppm <= 9.4) return map(ppm, 4.5, 9.4, 51, 100);   // Moderate
  else if (ppm <= 12.4) return map(ppm, 9.5, 12.4, 101, 150); // Unhealthy for Sensitive Groups
  else if (ppm <= 15.4) return map(ppm, 12.5, 15.4, 151, 200); // Unhealthy
  else if (ppm <= 30.4) return map(ppm, 15.5, 30.4, 201, 300); // Very Unhealthy
  else return map(ppm, 30.5, 50.0, 301, 500);                // Hazardous
}

// Function to calculate sound level in dB
float calculateDb(int analogValue) {
  float sensorVoltage = analogValue * (5.0 / 1023.0); // Convert analog value to voltage
  return 20 * log10(sensorVoltage / REFERENCE_VOLTAGE); // Convert to dB
}

void setup() {
  Wire.begin(); // Start I2C as Master
  Serial.begin(9600);

  // Initialize DHT sensors
  dht1.begin();
  dht2.begin();

  // Initialize Flame Sensor
  pinMode(FLAME_SENSOR_PIN, INPUT);

  // Initialize IR Sensor and Relay
  pinMode(IRSENSOR_PIN, INPUT);
  pinMode(RELAY_MODULE_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Initialize Relays for Cloud Variables
  pinMode(DISINFECTION_LIGHT_PIN, OUTPUT);
  pinMode(FRONT_LIGHT_PIN, OUTPUT);
  pinMode(MEDIBOT_LIGHT_PIN, OUTPUT);

  // Turn off all relays initially
  digitalWrite(DISINFECTION_LIGHT_PIN, LOW);
  digitalWrite(FRONT_LIGHT_PIN, LOW);
  digitalWrite(MEDIBOT_LIGHT_PIN, LOW);

  // IoT Cloud setup
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("System initialized with DHT11 sensors, MQ-7 CO monitoring, sound sensor, flame sensor, IR sanitizer system, and light controls.");
}

void loop() {
  ArduinoCloud.update(); // Sync with IoT Cloud

  // Read DHT11 sensors
  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();
  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  // Read MQ-7 sensor for CO ppm and AQI
  int analogValueMQ7 = analogRead(MQ7_PIN);
  float rsRoRatio = calculateRsRo(analogValueMQ7);
  ppm = calculatePPM(rsRoRatio);
  aqi = calculateAQI(ppm);

  // Check if gas levels are harmful
  if (ppm > PPM_THRESHOLD || aqi > AQI_THRESHOLD) { // Harmful levels detected
    gasAlertLight = true;  // Activate gas alert light
    Serial.println("Harmful gas detected! Gas alert activated.");
  } else {
    gasAlertLight = false; // Deactivate gas alert light
  }

  // Read Sound Sensor
  int analogValueSound = analogRead(SOUND_SENSOR_PIN);
  soundLevel = calculateDb(analogValueSound);
  soundExceeded = (soundLevel > SOUND_THRESHOLD_DB);

  // Check if sound levels are harmful
  if (soundLevel > SOUND_THRESHOLD_DB) {
    soundAlert = true; // Update cloud variable for noise alert
    Serial.println("Excessive noise detected! Sound alert activated.");
  } else {
    soundAlert = false; // Update cloud variable for noise alert
  }

  // Read Flame Sensor
  flameDetected = digitalRead(FLAME_SENSOR_PIN);

  // Read IR Sensor and control sanitizer
  sensor_value = digitalRead(IRSENSOR_PIN);
  if (sensor_value == HIGH) {
    digitalWrite(RELAY_MODULE_PIN, HIGH); // Activate pump
    digitalWrite(GREEN_LED_PIN, LOW);    // Turn off LED
    handSanitized = false; // Update cloud variable
    Serial.println("Obstacle Detected. Sanitizing...");
  } else {
    digitalWrite(RELAY_MODULE_PIN, LOW); // Deactivate pump
    digitalWrite(GREEN_LED_PIN, HIGH);   // Turn on LED
    handSanitized = true; // Update cloud variable
    Serial.println("No Obstacle Detected. Idle.");
  }

  // Log sensor data
  logSensorData(temp1, hum1, temp2, hum2);
}

// Helper function to log sensor data
void logSensorData(float temp1, float hum1, float temp2, float hum2) {
  Serial.print("Sensor 1 -> Temp: ");
  Serial.print(temp1);
  Serial.print("°C, Humidity: ");
  Serial.println(hum1);

  Serial.print("Sensor 2 -> Temp: ");
  Serial.print(temp2);
  Serial.print("°C, Humidity: ");
  Serial.println(hum2);

  Serial.print("CO PPM: ");
  Serial.print(ppm, 2);
  Serial.print(" | AQI: ");
  Serial.println(aqi);

  Serial.print("Sound Level: ");
  Serial.print(soundLevel, 2);
  Serial.println(" dB");

  Serial.print("Flame Detected: ");
  Serial.println(flameDetected ? "Yes" : "No");

  Serial.print("Hand Sanitized: ");
  Serial.println(handSanitized ? "Yes" : "No");

  Serial.print("Sound Alert: ");
  Serial.println(soundAlert ? "Yes" : "No");
}

// onChange functions for new cloud variables
void onDisinfectionLightChange() {
  if (disinfectionLight) {
    digitalWrite(DISINFECTION_LIGHT_PIN, LOW);
    Serial.println("Disinfection Light ON.");
  } else {
    digitalWrite(DISINFECTION_LIGHT_PIN, HIGH);
    Serial.println("Disinfection Light OFF.");
  }
}

void onFrontLightChange() {
  if (frontLight) {
    digitalWrite(FRONT_LIGHT_PIN, LOW);
    Serial.println("Front Light ON.");
  } else {
    digitalWrite(FRONT_LIGHT_PIN, HIGH);
    Serial.println("Front Light OFF.");
  }
}

void onMedibotLightChange() {
  if (medibotLight) {
    digitalWrite(MEDIBOT_LIGHT_PIN, LOW);
    Serial.println("Medibot Light ON.");
  } else {
    digitalWrite(MEDIBOT_LIGHT_PIN, HIGH);
    Serial.println("Medibot Light OFF.");
  }
}

void onGasAlertLightChange() {
  if (gasAlertLight) {
    Serial.println("Gas Alert Light ON.");
  } else {
    Serial.println("Gas Alert Light OFF.");
  }
}

void onSoundAlertChange() {
  Serial.println("Sound Alert status updated in the cloud.");
}

void onStepperbuttonChange() {
  if (stepperbutton) {
    sendCommand('O'); // Send 'O' for Open
  }
}

void onGatebackChange() {
  if (gateback) {
    sendCommand('C'); // Send 'C' for Close
  }
}

void sendCommand(char command) {
  Wire.beginTransmission(8); // Slave address is 8
  Wire.write(command);       // Send command ('O' or 'C')
  Wire.endTransmission();    // End the transmission
  Serial.print("Command sent: ");
  Serial.println(command);
}

void onHumSensor1Change() {
  Serial.println("humSensor1 changed!");
}

void onHumSensor2Change() {
  Serial.println("humSensor2 changed!");
}

void onTempSensor1Change() {
  Serial.println("tempSensor1 changed!");
}

void onTempSensor2Change() {
  Serial.println("tempSensor2 changed!");
}

void onPpmChange() {
  Serial.println("ppm changed!");
}

void onAqiChange() {
  Serial.println("aqi changed!");
}

void onSoundLevelChange() {
  Serial.println("soundLevel changed!");
}

void onSoundExceededChange() {
  Serial.println("soundExceeded changed!");
}

void onFlameDetectedChange() {
  Serial.println("flameDetected changed!");
}

void onHandSanitizedChange() {
  Serial.println("handSanitized cloud variable updated!");
}
