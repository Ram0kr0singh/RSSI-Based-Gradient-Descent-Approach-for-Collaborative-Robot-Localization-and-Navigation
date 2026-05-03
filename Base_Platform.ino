#include <ESP8266WiFi.h>

// ================= SETTINGS =================
const char* ssid = "Robot_Base_Station";   // 🔴 MUST MATCH robot code
const char* password = "12345678";         // Min 8 chars

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Base Station...");

  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("Base Station Ready!");
  Serial.print("SSID: ");
  Serial.println(ssid);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Nothing needed — just keep broadcasting WiFi
}