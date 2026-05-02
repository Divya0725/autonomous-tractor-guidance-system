/*
  ============================================================
  AUTONOMOUS TRACTOR GUIDANCE SYSTEM (ATGS)
  ------------------------------------------------------------
  Controller : Arduino Mega 2560
  
  MODULE 1 - Autonomous Navigation:
    - NEO-6M GPS Module         (Serial1: TX=18, RX=19)
    - MPU-6050 IMU              (I2C: SDA=20, SCL=21)
    - IR Line Sensors           (D2, D3)
    - HC-SR04 Ultrasonic        (TRIG=9, ECHO=10)
    - L298N Motor Driver        (D7,D8,D11,D12, ENA=D5, ENB=D13)
    - MG996R Servo (Steering)   (D6)

  MODULE 2 - Weed Detection & Removal:
    - OV7670 Camera             (via SPI/I2C)
    - MG996R Servo x3 (Arm)    (D22, D23, D24)

  MODULE 3 - Soil pH Monitoring:
    - SEN0161 Soil pH Sensor    (Analog A0)
    - Soil Moisture Sensor      (Analog A1)

  MODULE 4 - Pest Detection:
    - OV7670 Camera             (shared)
    - DHT22 Temperature Sensor  (D4)
  ------------------------------------------------------------
  Author : Divya M
  College: Chennai Institute of Technology
  ============================================================
*/

#include <Wire.h>
#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// ─── Pin Definitions ──────────────────────────────────────────
// Navigation
#define IR_LEFT        2
#define IR_RIGHT       3
#define DHT_PIN        4
#define STEERING_PIN   6
#define MOTOR_IN1      7
#define MOTOR_IN2      8
#define TRIG_PIN       9
#define ECHO_PIN       10
#define MOTOR_IN3      11
#define MOTOR_IN4      12
#define ENA_PIN        5
#define ENB_PIN        13

// Weed Arm Servos
#define ARM_BASE       22
#define ARM_ELBOW      23
#define ARM_CUTTER     24

// Soil Sensors
#define SOIL_PH_PIN    A0
#define SOIL_MOIST_PIN A1

// Thresholds
#define OBSTACLE_DIST  30     // cm
#define PH_LOW         5.5
#define PH_HIGH        7.5
#define MOIST_DRY      400
#define TEMP_PEST      30.0   // °C — pest-risk threshold

// ─── Objects ──────────────────────────────────────────────────
TinyGPSPlus      gps;
Adafruit_MPU6050 mpu;
DHT              dht(DHT_PIN, DHT22);
Servo            steeringServo;
Servo            armBase, armElbow, armCutter;

// ─── State ────────────────────────────────────────────────────
float  latitude    = 0, longitude  = 0;
float  soilPH      = 0, soilMoist  = 0;
float  temperature = 0, humidity   = 0;
long   obstacleDistance = 999;
bool   weedDetected     = false;
bool   pestRisk         = false;
String navStatus        = "IDLE";

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);   // GPS

  // Motor pins
  pinMode(MOTOR_IN1, OUTPUT); pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT); pinMode(MOTOR_IN4, OUTPUT);
  pinMode(ENA_PIN,   OUTPUT); pinMode(ENB_PIN,   OUTPUT);
  pinMode(IR_LEFT,   INPUT);  pinMode(IR_RIGHT,  INPUT);
  pinMode(TRIG_PIN,  OUTPUT); pinMode(ECHO_PIN,  INPUT);

  // Servos
  steeringServo.attach(STEERING_PIN);
  armBase.attach(ARM_BASE);
  armElbow.attach(ARM_ELBOW);
  armCutter.attach(ARM_CUTTER);
  steeringServo.write(90);   // Center steering
  armHome();

  // MPU6050
  if (!mpu.begin()) Serial.println("[ERROR] MPU6050 not found!");

  // DHT22
  dht.begin();

  Serial.println("=== ATGS System Started ===");
  Serial.println("Modules: Navigation | Weed | Soil | Pest");
}

// ─── Main Loop ────────────────────────────────────────────────
void loop() {
  module1_Navigation();
  module3_SoilMonitoring();
  module4_PestDetection();
  printStatus();
  delay(500);
}

// ══════════════════════════════════════════════════════════════
// MODULE 1: Autonomous Navigation
// ══════════════════════════════════════════════════════════════
void module1_Navigation() {
  // Read GPS
  while (Serial1.available()) gps.encode(Serial1.read());
  if (gps.location.isValid()) {
    latitude  = gps.location.lat();
    longitude = gps.location.lng();
  }

  // Read ultrasonic distance
  obstacleDistance = getDistance();

  // Read IR sensors
  int irLeft  = digitalRead(IR_LEFT);
  int irRight = digitalRead(IR_RIGHT);

  if (obstacleDistance < OBSTACLE_DIST) {
    // Obstacle — stop and turn
    stopMotors();
    navStatus = "OBSTACLE";
    delay(500);
    turnRight();
    delay(800);

  } else if (irLeft == LOW && irRight == HIGH) {
    // Drifting left — correct right
    steeringServo.write(110);
    moveForward();
    navStatus = "CORRECT_RIGHT";

  } else if (irLeft == HIGH && irRight == LOW) {
    // Drifting right — correct left
    steeringServo.write(70);
    moveForward();
    navStatus = "CORRECT_LEFT";

  } else {
    // Straight ahead
    steeringServo.write(90);
    moveForward();
    navStatus = "FORWARD";

    // Check for weed (simulated — replace with camera output)
    weedDetected = random(10) < 2;   // 20% chance for demo
    if (weedDetected) {
      stopMotors();
      navStatus = "WEED_REMOVE";
      module2_WeedRemoval();
    }
  }
}

// ══════════════════════════════════════════════════════════════
// MODULE 2: Weed Detection & Removal
// ══════════════════════════════════════════════════════════════
void module2_WeedRemoval() {
  Serial.println("[MOD2] Weed detected! Activating arm...");

  // Lower arm to weed
  armBase.write(45);   delay(600);
  armElbow.write(90);  delay(600);

  // Activate cutter
  armCutter.write(180); delay(1000);
  armCutter.write(0);   delay(500);

  // Return to home
  armHome();
  Serial.println("[MOD2] Weed removed. Resuming.");
  weedDetected = false;
}

void armHome() {
  armBase.write(0);
  armElbow.write(0);
  armCutter.write(0);
}

// ══════════════════════════════════════════════════════════════
// MODULE 3: Soil pH & Moisture Monitoring
// ══════════════════════════════════════════════════════════════
void module3_SoilMonitoring() {
  int rawPH    = analogRead(SOIL_PH_PIN);
  int rawMoist = analogRead(SOIL_MOIST_PIN);

  // Convert raw ADC to pH (SEN0161 calibration)
  soilPH    = 3.5 * (rawPH * 5.0 / 1023.0);
  soilMoist = map(rawMoist, 0, 1023, 0, 100);   // 0–100%

  if (soilPH < PH_LOW) {
    Serial.println("[MOD3] ALERT: Soil too ACIDIC! Add lime.");
  } else if (soilPH > PH_HIGH) {
    Serial.println("[MOD3] ALERT: Soil too ALKALINE! Add sulfur.");
  }

  if (soilMoist < 30) {
    Serial.println("[MOD3] ALERT: Soil DRY! Irrigation needed.");
  }
}

// ══════════════════════════════════════════════════════════════
// MODULE 4: Pest & Disease Detection
// ══════════════════════════════════════════════════════════════
void module4_PestDetection() {
  temperature = dht.readTemperature();
  humidity    = dht.readHumidity();

  if (isnan(temperature)) { temperature = 0; return; }

  // High temp + high humidity = pest risk
  pestRisk = (temperature > TEMP_PEST && humidity > 70);

  if (pestRisk) {
    Serial.println("[MOD4] PEST RISK DETECTED! Temp:" +
      String(temperature) + "C Hum:" + String(humidity) + "%");
    Serial.println("[MOD4] Recommend: Targeted pesticide spray.");
  }
}

// ─── Motor Control ────────────────────────────────────────────
void moveForward() {
  analogWrite(ENA_PIN, 180); analogWrite(ENB_PIN, 180);
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW); digitalWrite(MOTOR_IN4, LOW);
}

void turnRight() {
  steeringServo.write(130);
  analogWrite(ENA_PIN, 150); analogWrite(ENB_PIN, 150);
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
}

// ─── Ultrasonic Distance ──────────────────────────────────────
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// ─── Serial Status ────────────────────────────────────────────
void printStatus() {
  Serial.println("========== ATGS STATUS ==========");
  Serial.println("[MOD1] Nav     : " + navStatus);
  Serial.println("[MOD1] GPS     : " + String(latitude, 6) + ", " + String(longitude, 6));
  Serial.println("[MOD1] Obstacle: " + String(obstacleDistance) + " cm");
  Serial.println("[MOD2] Weed    : " + String(weedDetected ? "DETECTED" : "CLEAR"));
  Serial.println("[MOD3] Soil pH : " + String(soilPH, 2));
  Serial.println("[MOD3] Moisture: " + String(soilMoist, 1) + " %");
  Serial.println("[MOD4] Temp    : " + String(temperature, 1) + " C");
  Serial.println("[MOD4] Humidity: " + String(humidity, 1) + " %");
  Serial.println("[MOD4] PestRisk: " + String(pestRisk ? "YES" : "NO"));
  Serial.println("=================================");
}
