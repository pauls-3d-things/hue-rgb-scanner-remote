
#include "Adafruit_APDS9960.h"
#include "Arduino.h"
#include "Config.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <RGBConverter.h>

Adafruit_APDS9960 apds;
WiFiClient client;
const char *host = "192.168.178.38";
const String apiKey = "xZXN4VEGU5qPJMzVGJy0uxPtHbkmPcyxpv5yvtgf";
// cp config.h.example config.h
// then edit the constants for config
const char *wifiSSID = WIFI_SSID;
const char *wifiPass = WIFI_PASS;

#define NUM_LIGHTS 5
uint8_t hueRgbLights[NUM_LIGHTS] = {6, 7, 8, 11, 12};
uint8_t lightIndex = 0;

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
    while (true) {
      // fast flash no sensor
      setLights(100, 100);
      delay(100);
      setLights(0, 0);
      delay(100);
    }

  } else {
    Serial.println("Device initialized!");

    apds.enableColor(true);
    apds.enableProximity(true);
    apds.enableGesture(true);
  }

  uint8_t tries = 0;
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    if (tries % 10 == 0) {
      WiFi.begin(wifiSSID, wifiPass);
    }

    // slow flash connecting wifi
    delay(500);
    setLights(100, 0);
    delay(500);
    setLights(0, 100);
    tries++;
  }
  setLights(0, 0);
}

uint16_t r, g, b, c;
RGBConverter conv;

void sendRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t light) {
  double hsl[3];
  conv.rgbToHsl((byte)r, (byte)g, (byte)b, hsl);
  uint16_t hue = (uint16_t)(hsl[0] * 65535);

  if (client.connect(host, 80)) {

    String body = "{\"on\": true, \"hue\": " + String(hue) + "}";
    String path = "PUT /api/" + apiKey + "/lights/" + String(light) + "/state";
    client.print(path);
    client.println(" HTTP/1.1");
    client.println("Cache-Control: no-cache");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println();
    client.print(body);

    unsigned long currentMillis = millis(), previousMillis = millis();
    long interval = 2000;

    while (!client.available()) {

      if ((currentMillis - previousMillis) > interval) {

        Serial.println("Timeout");
        client.stop();
        return;
      }
      currentMillis = millis();
    }

    while (client.connected()) {
      if (client.available()) {
        char str = client.read();
        Serial.print(str);
      }
    }
    delay(100);
    client.stop();
  }
}

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

  if (apds.readProximity() < 20) {
    // uint8_t g = apds.readGesture();
    // switch (g) {
    // case APDS9960_RIGHT:
    // case APDS9960_DOWN:
    // case APDS9960_UP:
    // case APDS9960_LEFT:
    //   lightIndex = (lightIndex + 1) % NUM_LIGHTS;
    //   delay(50);
    //   setLights(50, 0);
    //   delay(50);
    //   setLights(0, 0);
    //   delay(50);
    //   setLights(50, 0);
    //   delay(50);
    //   setLights(0, 0);
    //   break;
    // default:
    //   //
    //   Serial.println("gesture: " + g);
    // }

    delay(50);
    return;
  }

  delay(500);
  uint16_t r, g, b, c;
  uint16_t r2, g2, b2, c2;
  // read bright samples
  getNormalizedColorData(&r, &g, &b, &c, 200, 16);
  delay(5);
  // read dark samples
  getNormalizedColorData(&r2, &g2, &b2, &c2, 10, 16);

  float maxval = 0;

  if (r > maxval)
    maxval = r;
  if (g > maxval)
    maxval = g;
  if (b > maxval)
    maxval = b;
  if (c > maxval)
    maxval = c;
  r = (uint16_t)(((r + r2) / maxval) * 255.0);
  g = (uint16_t)(((g + g2) / maxval) * 255.0);
  b = (uint16_t)(((b + b2) / maxval) * 255.0);
  c = (uint16_t)(((c + c2) / maxval) * 255.0);

  Serial.print("b: ");
  Serial.print(b > 255 ? 255 : b);

  Serial.print(" r: ");
  Serial.print(r > 255 ? 255 : r);

  Serial.print(" g: ");
  Serial.print(g > 255 ? 255 : g);
  Serial.println();

  delay(1500);
  sendRGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 6);
  sendRGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 7);
  sendRGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 8);
  sendRGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 11);
  sendRGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 12);
}