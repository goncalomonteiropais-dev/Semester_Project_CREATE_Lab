#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(D4, D5);  // SDA, SCL

  Serial.println("Initializing INA219...");

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 sensor");
    while (1);
  }

  Serial.println("INA219 initialized!");
}

void loop() {
  float current_mA = ina219.getCurrent_mA();

  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.println(" mA");

}