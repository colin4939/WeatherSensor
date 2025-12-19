#include <RH_ASK.h>
#include <SPI.h>

// 2000 bps to match TX. Choose the correct RX pin for your wiring.
// Constructor: RH_ASK(speed, rxPin, txPin, pttPin, pttInverted)
// We'll set rxPin=11 as a common default. Change if needed.
RH_ASK rf_driver(2000, 11, 255, 255, false);

// Must match transmitter struct exactly (types + order)
struct __attribute__((packed)) SensorPayload {
  uint16_t magic;
  uint16_t counter;
  uint16_t  tempC_x100;
  uint16_t  tempF_x100;
  uint16_t rh_x100;
  uint16_t inHg_x100;
};

static SensorPayload rx;

void setup() {
  Serial.begin(9600);

  if (!rf_driver.init()) {
    Serial.println(F("RF init failed"));
    while (1) {}
  }

  Serial.println(F("433MHz receiver ready"));
}

void loop() {
  uint8_t buflen = sizeof(rx);

  if (rf_driver.recv((uint8_t*)&rx, &buflen)) {
    // Basic checks
    if (buflen != sizeof(rx)) {
      Serial.print(F("Bad size: "));
      Serial.println(buflen);
      return;
    }
    if (rx.magic != 0xBEEF) {
      Serial.println(F("Bad magic"));
      return;
    }

    // Decode x100 to floats for printing
    float tC   = rx.tempC_x100 / 100.0f;
    float tF   = rx.tempF_x100 / 100.0f;
    float rh   = rx.rh_x100   / 100.0f;
    float inHg = rx.inHg_x100 / 100.0f;

    Serial.print(F("Pkt #"));
    Serial.print(rx.counter);
    Serial.print(F("  Temp: "));
    Serial.print(tC, 2);
    Serial.print(F(" C  "));
    Serial.print(tF, 2);
    Serial.print(F(" F  RH: "));
    Serial.print(rh, 2);
    Serial.print(F(" %  Pressure: "));
    Serial.print(inHg, 2);
    Serial.println(F(" inHg"));
  }
}
