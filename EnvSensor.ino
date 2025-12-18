#include <BMD31M090.h>
#include <forcedBMX280.h>
#include <Wire.h>
#include <RH_ASK.h>

RH_ASK rf_driver(2000, -1, 7, 5); //(Speed, RX, TX, PTT)
ForcedBME280Float climateSensor = ForcedBME280Float();

float g_temperatureFloat;    // raw temperature
float g_pressureFloat;       // raw pressure
float g_humidityFloat;       // raw humidity

float currentTempF;
float currentTempC;
float currentPre;
float currentHum;

#define BMD31M090_WIDTH   128        // BMD31M090 Module display width, in pixels
#define BMD31M090_HEIGHT  64         // BMD31M090 Module display height, in pixels
BMD31M090 BMD31(BMD31M090_WIDTH, BMD31M090_HEIGHT, &Wire);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  climateSensor.begin();
  BMD31.begin(0x3C);
  rf_driver.init();

  BMD31.setFont(FontTable_6X8);
  BMD31.drawString(0, displayROW3, (u8*)"Temp:");
  BMD31.drawString(65, displayROW3, (u8*)"C");
  BMD31.drawString(75, displayROW3, (u8*)"/");
  BMD31.drawString(120, displayROW3, (u8*)"F");
  BMD31.drawString(0, displayROW5, (u8*)"RH%:");
  BMD31.drawString(65, displayROW5, (u8*)"%");
  BMD31.drawString(0, displayROW7, (u8*)"BRO:");
  BMD31.drawString(65, displayROW7, (u8*)"inHg");
  delay(100); //Recommended initial setting delay value.
}

void loop() {
  //Get pressure value
  delay(1000);
  climateSensor.takeForcedMeasurement();

  g_temperatureFloat = climateSensor.getTemperatureCelsius();
  currentTempF = g_temperatureFloat/100*1.8+32;
  currentTempC = (currentTempF - 32)*5.0/9.0;
  Serial.println(" ");
  Serial.print("Temperature: ");
  Serial.print(currentTempC);
  Serial.print("°C");
  Serial.print("/");
  Serial.print(currentTempF);
  Serial.println("°F");
  
  g_humidityFloat = climateSensor.getRelativeHumidityAsFloat();
  currentHum = g_humidityFloat;
  Serial.print("Humidity: ");
  Serial.print(currentHum);
  Serial.println(" %");

  g_pressureFloat = climateSensor.getPressureAsFloat();
  currentPre = g_pressureFloat*0.02952998;
  Serial.print("Pressure: ");
  Serial.print(currentPre);
  Serial.println(" inHg");

  BMD31.setFont(FontTable_6X8);
  BMD31.drawNum(32, displayROW3, currentTempC, 5);
  BMD31.drawNum(85, displayROW3, currentTempF, 5);
  BMD31.drawNum(32, displayROW5, g_humidityFloat, 5);
  BMD31.drawNum(32, displayROW7, currentPre, 5);

  float SendPackage[3] = {g_temperatureFloat, g_humidityFloat, g_pressureFloat};
  rf_driver.send((uint8_t*)SendPackage, sizeof(SendPackage));
  rf_driver.waitPacketSent();

  delay(2000);
}