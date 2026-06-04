#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;
WebServer server(80);

// ===== WiFi =====
const char* ssid = "Gonz";
const char* password = "123456789";

// ===== Data =====
#define MAX_SAMPLES 1500

struct DataPoint {
  unsigned long t;
  float current;
  float voltage;
  float power;
  float energy;
};

DataPoint data[MAX_SAMPLES];
int indexData = 0;

bool recording = false;
bool motorState = false;

unsigned long startTime;
unsigned long lastTime;
float energy_J = 0;

// ===== HTML UI =====
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
button {font-size:20px; margin:10px; padding:15px;}
</style>
</head>
<body>
<h2>Motor Control Panel</h2>

<button onclick="fetch('/start')">Start Recording</button>
<button onclick="fetch('/stop')">Stop Recording</button><br>

<button onclick="fetch('/motor_on')">Motor ON</button>
<button onclick="fetch('/motor_off')">Motor OFF</button><br>

<a href="/download">Download CSV</a>

<p id="status"></p>

<script>
setInterval(async () => {
  let res = await fetch('/status');
  let txt = await res.text();
  document.getElementById("status").innerText = txt;
}, 1000);
</script>

</body>
</html>
)rawliteral";
}

// ===== Routes =====
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleStart() {
  indexData = 0;
  energy_J = 0;
  startTime = millis();
  lastTime = startTime;
  recording = true;
  server.send(200, "text/plain", "Recording started");
}

void handleStop() {
  recording = false;
  server.send(200, "text/plain", "Recording stopped");
}

void handleMotorOn() {
  motorState = true;
  digitalWrite(D2, HIGH); // future MOSFET
  server.send(200, "text/plain", "Motor ON");
}

void handleMotorOff() {
  motorState = false;
  digitalWrite(D2, LOW);
  server.send(200, "text/plain", "Motor OFF");
}

void handleStatus() {
  String s = "Recording: " + String(recording ? "ON" : "OFF");
  s += " | Motor: " + String(motorState ? "ON" : "OFF");
  server.send(200, "text/plain", s);
}

void handleDownload() {
  String s = "time_ms,current_mA,voltage_V,power_W,energy_J\n";
  for (int i = 0; i < indexData; i++) {
    s += String(data[i].t) + "," +
         String(data[i].current) + "," +
         String(data[i].voltage) + "," +
         String(data[i].power) + "," +
         String(data[i].energy) + "\n";
  }
  server.send(200, "text/csv", s);
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(D4, D5);

  pinMode(D2, OUTPUT); // motor control pin

  if (!ina219.begin()) {
    Serial.println("INA219 error");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/motor_on", handleMotorOn);
  server.on("/motor_off", handleMotorOff);
  server.on("/status", handleStatus);
  server.on("/download", handleDownload);

  server.begin();
}

// ===== Loop =====
void loop() {
  server.handleClient();

  if (!recording) return;

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float current_mA = ina219.getCurrent_mA();
  float voltage = ina219.getBusVoltage_V();
  float current_A = current_mA / 1000.0;
  float power = voltage * current_A;

  energy_J += power * dt;

  if (indexData < MAX_SAMPLES) {
    data[indexData++] = {now - startTime, current_mA, voltage, power, energy_J};
  }

  delay(50);
}