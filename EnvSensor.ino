#include <Wire.h>
#include <BMD31M090.h>
#include <forcedBMX280.h>

ForcedBME280Float climateSensor;

// Display
BMD31M090 BMD31(128, 64, &Wire);

static char buf[12];   // small shared buffer

// Format value scaled by 100 (xx.yy)
static void fmt_x100(int32_t v_x100) {
  bool neg = (v_x100 < 0);
  if (neg) v_x100 = -v_x100;

  int32_t whole = v_x100 / 100;
  int32_t frac  = v_x100 % 100;

  char *p = buf;
  if (neg) *p++ = '-';

  // convert whole part
  char tmp[8];
  uint8_t i = 0;
  do {
    tmp[i++] = '0' + (whole % 10);
    whole /= 10;
  } while (whole && i < sizeof(tmp));

  while (i--) *p++ = tmp[i];

  *p++ = '.';
  *p++ = '0' + (frac / 10);
  *p++ = '0' + (frac % 10);
  *p = '\0';
}

static void draw_x100(uint8_t x, uint8_t y, int32_t v_x100) {
  fmt_x100(v_x100);
  BMD31.drawString(x, y, (u8*)buf);
}

void setup() {
  Wire.begin();
  climateSensor.begin();
  BMD31.begin(0x3C);

  BMD31.setFont(FontTable_6X8);

  // Static labels (draw once)
  BMD31.drawString(0,  displayROW1, (u8*)"Env Sensor");
  BMD31.drawString(0,  displayROW3, (u8*)"Temp:");
  BMD31.drawString(65, displayROW3, (u8*)"C");
  BMD31.drawString(0,  displayROW5, (u8*)"RH%:");
  BMD31.drawString(65, displayROW5, (u8*)"%");
  BMD31.drawString(0,  displayROW7, (u8*)"BRO:");
  BMD31.drawString(65, displayROW7, (u8*)"inHg");

  delay(100);
}

void loop() {
  delay(1000);
  climateSensor.takeForcedMeasurement();

  // Temperature (assumed already scaled x100 by library)
  int32_t tempC_x100 = (int32_t)climateSensor.getTemperatureCelsius();

  // Humidity (% x100)
  float hum_f = climateSensor.getRelativeHumidityAsFloat();
  int32_t hum_x100 = (int32_t)(hum_f * 100.0f + 0.5f);

  // Pressure -> inHg x100
  // If pressure is in hPa:
  float pres_f = climateSensor.getPressureAsFloat();
  int32_t presInHg_x100 = (int32_t)(pres_f * 2.952998f + 0.5f);

  draw_x100(32, displayROW3, tempC_x100);
  draw_x100(32, displayROW5, hum_x100);
  draw_x100(32, displayROW7, presInHg_x100);

  delay(2000);
}
