#include <FastLED.h>
// #include <M5Atom.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

const IPAddress ip(192, 168, 88, 3);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress serverIP1(192, 168, 88, 1);
const IPAddress serverIP2(192, 168, 88, 2);

const int UDP_PORT = 55555;

WiFiUDP udp;

uint8_t receive_state = 0;
uint8_t control_state = 0;  // 0: stop 1: run

// M5Atom
const int m5buttonPin = 39;
CRGB m5leds[1];

// External Button
const int buttonPin = 32;
const uint8_t ledPin = 26;
const uint8_t ledNum = 48;
CRGB leds[ledNum];

bool heartbeat = false;

void setup() {
  // M5.begin(true, false, true);
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(m5buttonPin, INPUT_PULLUP);

  FastLED.addLeds<NEOPIXEL, 27>(m5leds, 1);
  FastLED.addLeds<NEOPIXEL, ledPin>(leds, ledNum);
  FastLED.setBrightness(20);
  FastLED.clear();

  button_led_all(CRGB(255, 255, 255));
  init_wifi_client();
  button_led_all(CRGB(255, 0, 0));

  delay(500);  // TODO: task起動待ち。せまふぉ？
}

void button_led_all(CRGB c) {
  m5leds[0] = c;
  for (int i = 0; i < ledNum; i++) leds[i] = c;
  FastLED.show();
}

void soft_blink(CRGB base_c, float hz = 1.0, float max = 1.0, float min = 0.0) {
  float t = (float)millis() / 1000;
  float gain = (sin(t * 3.14 * hz) + 1) / 2 * (max - min) + min;
  gain = constrain(gain, 0, 1);
  CRGB c(base_c.r * gain, base_c.g * gain, base_c.b * gain);
  // = base_c * (uint8_t)(0xff * gain);
  button_led_all(c);
}

void rotate(CRGB c, int n = 4, float rps = 0.5, float max = 1.0,
            float min = 0.0) {
  float t = 3.14 * 2 * millis() / 1000 * rps;
  for (int i = 0; i < 24; i++) {
    float angle = 3.14 * 2 * n * i / 24 + t;
    float gain = (sin(angle) + 1) / 2 * (max - min) + min;
    gain = constrain(gain, 0, 1);
    leds[i] = CRGB(c.r * gain, c.g * gain, c.b * gain);
  }
  for (int i = 0; i < 16; i++) {
    float angle = 3.14 * 2 * n * i / 16 + t;
    float gain = (sin(angle) + 1) / 2 * (max - min) + min;
    gain = constrain(gain, 0, 1);
    leds[i + 24] = CRGB(c.r * gain, c.g * gain, c.b * gain);
  }
  for (int i = 0; i < 8; i++) {
    float angle = 3.14 * 2 * n * i / 8 + t;
    float gain = (sin(angle) + 1) / 2 * (max - min) + min;
    gain = constrain(gain, 0, 1);
    leds[i + 40] = CRGB(c.r * gain, c.g * gain, c.b * gain);
  }

  m5leds[0] = leds[0];
  FastLED.show();
}

void hue_rotate(float rps = 0.5) {
  float t = rps * millis() / 1000;
  uint8_t hue;
  for (int i = 0; i < 24; i++) {
    hue = 256 * ((float)i / 24 + t);
    leds[i] = CHSV(hue, 255, 255);
  }
  for (int i = 0; i < 16; i++) {
    hue = 256 * ((float)i / 16 + t);
    leds[i + 24] = CHSV(hue, 255, 255);
  }
  for (int i = 0; i < 8; i++) {
    hue = 256 * ((float)i / 8 + t);
    leds[i + 40] = CHSV(hue, 255, 255);
  }

  m5leds[0] = leds[0];
  FastLED.show();
}

void init_wifi_client() {
  WiFi.config(ip, gateway, subnet, dns);
  delay(10);

  // We start by connecting to a WiFi network -----------------------------
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  receiveFromCar();

  control_state = 0;
  if (!digitalRead(buttonPin) || !digitalRead(m5buttonPin)) {
    control_state = 1;  // Run
    // soft_blink(CRGB(0, 255, 0), 2, 1, 0.7);
    soft_blink(CRGB(255, 0, 255), 4, 1, 0.2);
    // rotate(CRGB(255, 0, 255), 4, 0.5, 1, -2);
    // hue_rotate(0.5);
  } else {
    hue_rotate(0.5);
    // soft_blink(CRGB(255, 255, 0), 0.5, 1, 0.2);
    // soft_blink(CRGB(0, 0, 255));
  }

  sendControlState(serverIP1);
  sendControlState(serverIP2);
  delay(10);  // TODO: remove
}

void receiveFromCar() {
  if (!heartbeat) {
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
      receive_state = udp.read();
      heartbeat = true;
    }
  }
}

void sendControlState(const IPAddress ip) {
  udp.beginPacket(ip, UDP_PORT);
  udp.write(control_state);
  udp.endPacket();
}