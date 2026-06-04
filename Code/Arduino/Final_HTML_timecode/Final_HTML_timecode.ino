#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <time.h>
#include <sys/time.h>

Adafruit_INA219 ina219;
WebServer server(80);

// ===== WiFi =====
const char* ssid = "Gonz";
const char* password = "123456789";

// ===== NTP / Real time =====
const char* ntpServer = "pool.ntp.org";

// ===== Timezone (POSIX TZ) =====
// This example uses Central European Time with automatic DST (CET/CEST).
// Replace with your region's TZ string if needed.
const char* tzString = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// ===== Data =====
#define MAX_SAMPLES 1500

struct DataPoint {
  unsigned long t_ms;     // relative time since recording start
  uint64_t epoch_ms;      // absolute wall-clock time in milliseconds
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
uint64_t startEpochMs;
float energy_J = 0;

// ===== Helpers =====
uint64_t getEpochMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000ULL);
}

String epochToIsoString(uint64_t epoch_ms) {
  time_t seconds = (time_t)(epoch_ms / 1000ULL);
  struct tm timeinfo;
  localtime_r(&seconds, &timeinfo); // localtime uses TZ rules set via setenv/tzset

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

  unsigned int ms = (unsigned int)(epoch_ms % 1000ULL);
  char result[40];
  snprintf(result, sizeof(result), "%s.%03u", buffer, ms);
  return String(result);
}

// ===== HTML UI =====
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<style>
button {font-size:20px; margin:10px; padding:15px;}
canvas {max-width:100%;}
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

<canvas id="chart"></canvas>

<script>
let ctx = document.getElementById('chart').getContext('2d');

let data = {
  labels: [],
  datasets: [
    {
      label: 'Current (mA)',
      data: [],
      borderWidth: 2,
      fill: false
    },
    {
      label: 'Power (W)',
      data: [],
      borderWidth: 2,
      fill: false
    }
  ]
};

let chart = new Chart(ctx, {
  type: 'line',
  data: data,
  options: {
    animation: false,
    scales: {
      x: { display: false }
    }
  }
});

// Status update
setInterval(async () => {
  let res = await fetch('/status');
  let txt = await res.text();
  document.getElementById("status").innerText = txt;
}, 1000);

// Live data update
setInterval(async () => {
  let res = await fetch('/data');
  let json = await res.json();

  let now = Date.now();

  data.labels.push(now);
  data.datasets[0].data.push(json.current);
  data.datasets[1].data.push(json.power);

  if (data.labels.length > 60) {
    data.labels.shift();
    data.datasets[0].data.shift();
    data.datasets[1].data.shift();
  }

  chart.update();
}, 200);

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
  startEpochMs = getEpochMs();
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
  digitalWrite(D2, HIGH);
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

// ===== LIVE DATA ENDPOINT =====
void handleData() {
  float current_mA = ina219.getCurrent_mA();
  float voltage = ina219.getBusVoltage_V();
  float power = (current_mA / 1000.0) * voltage;

  String json = "{";
  json += "\"current\":" + String(current_mA) + ",";
  json += "\"power\":" + String(power);
  json += "}";

  server.send(200, "application/json", json);
}

// ===== CSV DOWNLOAD =====
void handleDownload() {
  String s = "time_ms,epoch_ms,iso_time,current_mA,voltage_V,power_W,energy_J\n";
  for (int i = 0; i < indexData; i++) {
    s += String(data[i].t_ms) + "," +
         String(data[i].epoch_ms) + "," +
         epochToIsoString(data[i].epoch_ms) + "," +
         String(data[i].current, 3) + "," +
         String(data[i].voltage, 3) + "," +
         String(data[i].power, 3) + "," +
         String(data[i].energy, 3) + "\n";
  }
  server.send(200, "text/csv", s);
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(D4, D5);
  pinMode(D2, OUTPUT);

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

  // Configure NTP (UTC) and set TZ rules for localtime
  configTime(0, 0, ntpServer);          // keep system time in UTC
  setenv("TZ", tzString, 1);            // apply POSIX TZ rules (e.g., CET/CEST)
  tzset();                              // refresh libc timezone handling

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.println("Waiting for NTP time...");
  }

  Serial.println("Time synchronized");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/motor_on", handleMotorOn);
  server.on("/motor_off", handleMotorOff);
  server.on("/status", handleStatus);
  server.on("/download", handleDownload);
  server.on("/data", handleData);

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
    data[indexData++] = {
      now - startTime,
      startEpochMs + (uint64_t)(now - startTime),
      current_mA,
      voltage,
      power,
      energy_J
    };
  }

  delay(50);
}