#include <Wire.h>
#include <BMD31M090.h>
#include <RH_ASK.h>
#include <SPI.h>

RH_ASK rf_driver(2000, 11, 0, 255, false);
BMD31M090 BMD31(128, 64, &Wire);

struct __attribute__((packed)) SensorPayload {
  uint16_t magic;
  uint16_t counter;
  uint16_t tempC_x100;
  uint16_t tempF_x100;
  uint16_t rh_x100;
  uint16_t inHg_x100;
};

static SensorPayload rx;
static char numBuf[10];  // reused buffer

// Convert unsigned x100 value to "ddd.dd" (no floats)
static char* u16x100_to_str(uint16_t v, char* out) {
  uint16_t ip = v / 100;
  uint8_t  fp = v % 100;

  // itoa for integer part
  char tmp[6];
  itoa(ip, tmp, 10);

  // copy tmp -> out
  char* p = out;
  for (char* t = tmp; *t; ++t) *p++ = *t;

  // add . and 2 digits
  *p++ = '.';
  *p++ = char('0' + (fp / 10));
  *p++ = char('0' + (fp % 10));
  *p = '\0';

  return out;
}

static void drawU16x100(uint8_t x, uint8_t y, uint16_t v, uint8_t minWidth = 0) {
  u16x100_to_str(v, numBuf);

  // simple left-padding with spaces if you want columns to stay aligned
  uint8_t len = strlen(numBuf);
  while (len < minWidth && len < sizeof(numBuf) - 1) {
    // shift right by 1
    for (int i = len; i >= 0; --i) numBuf[i + 1] = numBuf[i];
    numBuf[0] = ' ';
    len++;
  }

  BMD31.drawString(x, y, (u8*)numBuf);
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  BMD31.begin(0x3C);
  BMD31.setFont(FontTable_6X8);

  // Draw static labels once
  BMD31.drawString(0,  displayROW1, (u8*)"Env Sensor");
  BMD31.drawString(0,  displayROW3, (u8*)"Temp:");
  BMD31.drawString(65, displayROW3, (u8*)"C");
  BMD31.drawString(115, displayROW3, (u8*)"F");
  BMD31.drawString(0,  displayROW5, (u8*)"RH%:");
  BMD31.drawString(65, displayROW5, (u8*)"%");
  BMD31.drawString(0,  displayROW7, (u8*)"BRO:");
  BMD31.drawString(65, displayROW7, (u8*)"inHg");

  if (!rf_driver.init()) {
    Serial.println(F("RF init failed"));
    while (1) {}
  }

  Serial.println(F("433MHz receiver ready"));
}

void loop() {
  uint8_t buflen = sizeof(rx);

  if (rf_driver.recv((uint8_t*)&rx, &buflen)) {
    if (buflen != sizeof(rx)) {
      Serial.print(F("Bad size: "));
      Serial.println(buflen);
      return;
    }
    if (rx.magic != 0xBEEF) {
      Serial.println(F("Bad magic"));
      return;
    }

    // Serial print without floats
    Serial.print(F("Pkt #"));
    Serial.print(rx.counter);
    Serial.print(F(" Temp: "));
    Serial.print(u16x100_to_str(rx.tempC_x100, numBuf));
    Serial.print(F(" C  "));
    Serial.print(u16x100_to_str(rx.tempF_x100, numBuf));
    Serial.print(F(" F  RH: "));
    Serial.print(u16x100_to_str(rx.rh_x100, numBuf));
    Serial.print(F(" %  Pressure: "));
    Serial.print(u16x100_to_str(rx.inHg_x100, numBuf));
    Serial.println(F(" inHg"));

    // Display (minWidth keeps columns steady; adjust as you like)
    drawU16x100(28, displayROW3, rx.tempC_x100, 6);
    drawU16x100(75, displayROW3, rx.tempF_x100, 6);
    drawU16x100(28, displayROW5, rx.rh_x100,   6);
    drawU16x100(28, displayROW7, rx.inHg_x100, 6);
  }

  delay(1000);
}
