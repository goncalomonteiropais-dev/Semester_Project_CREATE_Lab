#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

unsigned long startTime;
unsigned long lastTime;
float energy_J = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(D4, D5);

  if (!ina219.begin()) {
    Serial.println("ERROR: INA219 not found");
    while (1);
  }

  startTime = millis();
  lastTime = startTime;

  // CSV header
  Serial.println("time_ms,current_mA,voltage_V,power_W,energy_J");
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float current_mA = ina219.getCurrent_mA();
  float voltage = ina219.getBusVoltage_V();

  float current_A = current_mA / 1000.0;
  float power_W = voltage * current_A;

  energy_J += power_W * dt;

  unsigned long t = now - startTime;

  // CSV output
  Serial.print(t);
  Serial.print(",");
  Serial.print(current_mA);
  Serial.print(",");
  Serial.print(voltage);
  Serial.print(",");
  Serial.print(power_W);
  Serial.print(",");
  Serial.println(energy_J);

  delay(50); // ~20 Hz
}