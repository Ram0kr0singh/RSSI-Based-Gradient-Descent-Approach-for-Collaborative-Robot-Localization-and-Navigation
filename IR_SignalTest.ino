// ---------------- IR SENSOR TEST ----------------

#define IR_PIN 15   // Change if your wiring is different

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  Serial.println("IR Sensor Test Started...");
}

void loop() {

  int irState = digitalRead(IR_PIN);

  if (irState == LOW) {
    Serial.println("Obstacle Detected!");
  } else {
    Serial.println("No Obstacle");
  }

  delay(500);
}