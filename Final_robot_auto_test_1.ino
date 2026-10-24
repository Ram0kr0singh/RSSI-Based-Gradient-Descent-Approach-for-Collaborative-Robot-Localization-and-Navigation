#include <ESP8266WiFi.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ================== USER SETTINGS ==================
const char* targetSSID = "Robot_Base_Station";  // MUST MATCH base_final.ino

int closeRSSI = -55;  // Stop threshold (dBm) — tune for your environment
int midRSSI   = -70;  // Medium speed threshold (dBm) — tune for your environment

// ================== MOTOR PINS ==================
#define PWMA D5   // Left motor PWM enable
#define AIN1 D6   // Left motor direction 1
#define AIN2 D7   // Left motor direction 2

#define PWMB D3   // Right motor PWM enable
#define BIN1 D8   // Right motor direction 1
#define BIN2 3    // Right motor direction 2 (RX pin — note conflict with Serial)

// ================== ULTRASONIC ==================
#define TRIG D0
#define ECHO D4

long duration;
int  distance;

// ================== MPU ==================
int16_t ax, ay, az;
int16_t gx, gy, gz;
int baseAngle = 0;

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial.println("STARTING...");

  // Motor pins
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // Sensor pins
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);

  // WiFi — station mode, scan only (no connection needed)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // MPU-6050 initialization (D2 = SDA, D1 = SCL)
  Wire.begin(D2, D1);
  mpu.initialize();

  // Calibrate base angle — keep robot still and level for 2 s
  Serial.println("Calibrating MPU...");
  delay(2000);
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  baseAngle = ay;  // Store initial tilt as reference

  Serial.println("Robot Ready!");
}

// ================== ULTRASONIC DISTANCE ==================
int getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  duration = pulseIn(ECHO, HIGH);
  return duration * 0.034 / 2;
}

// ================== RSSI SCAN ==================
int getRSSI() {
  int n    = WiFi.scanNetworks();
  int rssi = -100;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == targetSSID) {
      rssi = WiFi.RSSI(i);
    }
  }
  return rssi;
}

// ================== MOTOR CONTROL ==================
void moveForward(int speedA, int speedB) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speedA);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speedB);
}

void stopMotors() {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

void turnRight(int timeMs) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
  delay(timeMs);
}

void turnLeft(int timeMs) {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  delay(timeMs);
}

// ================== MPU STRAIGHT-LINE CORRECTION ==================
void moveStraight(int baseSpeed) {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  int error      = ay - baseAngle;
  int correction = error / 500;          // tune divisor for sensitivity
  int leftSpeed  = constrain(baseSpeed - correction, 100, 255);
  int rightSpeed = constrain(baseSpeed + correction, 100, 255);
  moveForward(leftSpeed, rightSpeed);
}

// ================== MAIN LOOP ==================
void loop() {
  int dist = getDistance();
  int rssi = getRSSI();

  Serial.print("Dist: "); Serial.print(dist);
  Serial.print(" cm | RSSI: "); Serial.print(rssi);
  Serial.println(" dBm");

  // === OBSTACLE AVOIDANCE (priority over navigation) ===
  if (dist < 20 && dist > 0) {
    stopMotors();
    delay(300);
    turnRight(400);  // Adjust duration for your robot turn radius
    return;
  }

  // === GRADIENT DESCENT RSSI NAVIGATION ===
  if (rssi > closeRSSI) {
    stopMotors();
    Serial.println(">>> TARGET REACHED - Converged! <<<");
  }
  else if (rssi > midRSSI) {
    moveStraight(130);  // Medium speed — close to base
  }
  else {
    moveStraight(200);  // Fast speed — far from base
  }

  delay(200);
}
