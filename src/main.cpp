
#include "Adafruit_APDS9960.h"
#include "Arduino.h"
Adafruit_APDS9960 apds;

void setLights(uint8_t left, uint8_t right) {
  analogWrite(D5, left);
  analogWrite(D6, right);
}

void setup() {
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  setLights(0, 0);

  Serial.begin(115200);

  if (!apds.begin()) {
    Serial.println("failed to initialize device! Please check your wiring.");
  } else
    Serial.println("Device initialized!");

  // enable color sensign mode
  apds.enableColor(true);
  apds.enableProximity(true);
}

uint16_t r, g, b, c;

void getNormalizedColorData(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c,
                            uint8_t lights, uint8_t samples) {
  uint16_t rt, gt, bt, ct;
  float r1, g1, b1, c1;
  r1 = 0;
  g1 = 0;
  b1 = 0;
  c1 = 0;

  setLights(lights, lights);
  while (samples > 0) {
    while (!apds.colorDataReady()) {
      delay(5);
    }
    apds.getColorData(&rt, &gt, &bt, &ct);
    r1 += rt / (samples * 1.0);
    g1 += gt / (samples * 1.0);
    b1 += bt / (samples * 1.0);
    c1 += ct / (samples * 1.0);
    samples--;
  }

  setLights(0, 0);

  *r = (uint16_t)r1;
  *g = (uint16_t)g1;
  *b = (uint16_t)b1;
  *c = (uint16_t)c1;
}

void loop() {
  delay(500);
  if (apds.readProximity() < 20) {
    return;
  }

  uint16_t r, g, b, c;
  uint16_t r2, g2, b2, c2;
  // read bright samples
  getNormalizedColorData(&r, &g, &b, &c, 200, 16);
  delay(5);
  // read dark samples
  getNormalizedColorData(&r2, &g2, &b2, &c2, 50, 16);

  float maxval = 0;

  if (r > maxval)
    maxval = r;
  if (g > maxval)
    maxval = g;
  if (b > maxval)
    maxval = b;
  if (c > maxval)
    maxval = c;
  r = (uint16_t)((r / maxval) * 255.0);
  g = (uint16_t)((g / maxval) * 255.0);
  b = (uint16_t)((b / maxval) * 255.0);
  c = (uint16_t)((c / maxval) * 255.0);

  r2 = (uint16_t)((r2 / maxval) * 255.0);
  g2 = (uint16_t)((g2 / maxval) * 255.0);
  b2 = (uint16_t)((b2 / maxval) * 255.0);
  c2 = (uint16_t)((c2 / maxval) * 255.0);

  Serial.print("b: ");
  Serial.print(b > 255 ? 255 : b);

  Serial.print(" r: ");
  Serial.print(r > 255 ? 255 : r);

  Serial.print(" g: ");
  Serial.print(g > 255 ? 255 : g);

  Serial.print(" bb: ");
  Serial.print(b2 > 255 ? 255 : b2);

  Serial.print(" rr: ");
  Serial.print(r2 > 255 ? 255 : r2);

  Serial.print(" gg: ");
  Serial.print(g2 > 255 ? 255 : g2);

  // Serial.print(" p: ");
  // Serial.print(apds.readProximity());
  // Serial.print(" clear: ");
  // Serial.println(c);
  Serial.println();
}