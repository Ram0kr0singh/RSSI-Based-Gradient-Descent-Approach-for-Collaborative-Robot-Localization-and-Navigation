#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ================== WIFI ==================
const char* ssid = "Pixel_4167";
const char* password = "1q2w3e4r";

ESP8266WebServer server(80);

// ================== USER SETTINGS ==================
const char* targetSSID = "Robot_Base_Station";
int closeRSSI = -55;

// ================== MOTOR PINS ==================
#define PWMA D5
#define AIN1 D6
#define AIN2 D7

#define PWMB D2
#define BIN1 D1
#define BIN2 D0

// ================== MODE ==================
bool manualMode = false;
bool systemStarted = false;
String command = "STOP";

// ================== SPEED SETTINGS ==================
int baseSpeed = 250;   // 🔥 GLOBAL SLOW SPEED (used everywhere)

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ================== WEB ROUTES ==================
  server.on("/", handleRoot);

  server.on("/start", [](){ systemStarted = true; });
  server.on("/stop_all", [](){ systemStarted = false; stopMotors(); });

  server.on("/forward", [](){ command = "F"; manualMode = true; });
  server.on("/back", [](){ command = "B"; manualMode = true; });
  server.on("/left", [](){ command = "L"; manualMode = true; });
  server.on("/right", [](){ command = "R"; manualMode = true; });
  server.on("/stop", [](){ command = "S"; manualMode = true; });

  server.on("/auto", [](){ manualMode = false; });

  server.begin();
}

// ================== WEB PAGE ==================
void handleRoot() {
  String page = R"rawliteral(
  <html>
  <head>
  <style>
  body { background:black; color:white; text-align:center; font-family:Arial; }
  button {
    width:120px; height:50px; font-size:18px; margin:8px;
    background:#333; color:white; border:none; border-radius:8px;
  }
  </style>

  <script>
  function send(cmd){ fetch("/"+cmd); }
  function startCmd(cmd){ send(cmd); }
  function stopCmd(){ send("stop"); }
  </script>

  </head>
  <body>

  <h2>Robot Control</h2>

  <button onclick="send('start')">START</button>
  <button onclick="send('stop_all')">STOP ALL</button>

  <br><br>

  <button onmousedown="startCmd('forward')" onmouseup="stopCmd()"
          ontouchstart="startCmd('forward')" ontouchend="stopCmd()">Forward</button>

  <br>

  <button onmousedown="startCmd('left')" onmouseup="stopCmd()"
          ontouchstart="startCmd('left')" ontouchend="stopCmd()">Left</button>

  <button onclick="send('stop')">Stop</button>

  <button onmousedown="startCmd('right')" onmouseup="stopCmd()"
          ontouchstart="startCmd('right')" ontouchend="stopCmd()">Right</button>

  <br>

  <button onmousedown="startCmd('back')" onmouseup="stopCmd()"
          ontouchstart="startCmd('back')" ontouchend="stopCmd()">Back</button>

  <br><br>

  <button onclick="send('auto')">Auto Mode</button>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

// ================== RSSI ==================
int getRSSI() {
  int n = WiFi.scanNetworks();
  int rssi = -100;

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == targetSSID) {
      rssi = WiFi.RSSI(i);
    }
  }
  return rssi;
}

// ================== MOTOR ==================
void moveForward(int sA, int sB) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, sA);
  analogWrite(PWMB, sB);
}

void moveBackward() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, baseSpeed);
  analogWrite(PWMB, baseSpeed);
}

void turnLeft() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);

  analogWrite(PWMA, baseSpeed);
  analogWrite(PWMB, baseSpeed);
}

void turnRight() {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  analogWrite(PWMA, baseSpeed);
  analogWrite(PWMB, baseSpeed);
}

void stopMotors() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}

// ================== LOOP ==================
void loop() {
  server.handleClient();

  // WAIT FOR START
  if (!systemStarted) {
    stopMotors();
    return;
  }

  // MANUAL MODE
  if (manualMode) {
    if (command == "F") moveForward(baseSpeed, baseSpeed - 20);
    else if (command == "B") moveBackward();
    else if (command == "L") turnLeft();
    else if (command == "R") turnRight();
    else if (command == "S") stopMotors();
    return;
  }

  // AUTO MODE (SAME SPEED RANGE)
  int rssi = getRSSI();

  if (rssi > closeRSSI) {
    stopMotors();
  } else {
    int speed = map(rssi, -90, closeRSSI, 300, 200); // 🔥 slower range
    speed = constrain(speed, 200, 300);

    moveForward(speed, speed - 20);
  }

  delay(200);
}