#include <Wire.h>
#include <BMD31M090.h>
#include <forcedBMX280.h>
#include <RH_ASK.h>
#include <SPI.h>

ForcedBME280Float climateSensor;
RH_ASK rf_driver(2000, 0, 7, 255, false); // 2000 bps, RX unused, TX pin 7

// Display
BMD31M090 BMD31(128, 64, &Wire);

static char buf[12];   // small shared buffer


// ---------- 433MHz payload ----------
struct __attribute__((packed)) SensorPayload {
  uint16_t magic;      // for quick validation on receiver
  uint16_t counter;    // increments each send
  int16_t  tempC_x100; // °C * 100
  int16_t  tempF_x100; // °F * 100
  uint16_t rh_x100;    // %RH * 100
  uint16_t inHg_x100;  // inHg * 100
};

static SensorPayload tx;
static uint16_t txCounter = 0;


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
  rf_driver.init(); // returns bool in some versions; ignoring here for brevity

  BMD31.setFont(FontTable_6X8);

  // Static labels (draw once)
  BMD31.drawString(0,  displayROW1, (u8*)"Env Sensor");
  BMD31.drawString(0,  displayROW3, (u8*)"Temp:");
  BMD31.drawString(65, displayROW3, (u8*)"C");
  BMD31.drawString(115, displayROW3, (u8*)"F");
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

  // Celsius x100 -> Fahrenheit x100
  int32_t tempF_x100 = (tempC_x100 * 9) / 5 + 3200;

  // Pressure: confirm what your library returns!
  // If it returns hPa, inHg = hPa * 0.02952998
  // So inHg_x100 = hPa * 2.952998
  float pres_hPa = climateSensor.getPressureAsFloat();
  int32_t inHg_x100 = (int32_t)(pres_hPa * 2.952998f + 0.5f);

  draw_x100(32, displayROW3, tempC_x100);
  draw_x100(82, displayROW3, tempF_x100);
  draw_x100(32, displayROW5, hum_x100);
  draw_x100(32, displayROW7, inHg_x100);

  // --- Build RF packet ---
  tx.magic      = 0xBEEF;
  tx.counter    = txCounter++;
  tx.tempC_x100 = (int16_t)tempC_x100;
  tx.tempF_x100 = (int16_t)tempF_x100;
  tx.rh_x100    = (uint16_t)hum_x100;
  tx.inHg_x100  = (uint16_t)inHg_x100;

  // --- Send over 433 MHz ---
  rf_driver.send((uint8_t*)&tx, sizeof(tx));
  rf_driver.waitPacketSent();

  delay(2000);
}
