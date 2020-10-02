#include <FastLED.h>
// #include <M5Atom.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include "config.h"

#define USE_SOFT_AP
const IPAddress ip(192, 168, 88, 1);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress clientIP(192, 168, 88, 2);
const IPAddress clientIP2(192, 168, 88, 3);

const int UDP_PORT = 55555;

WiFiUDP udp;

// TODO: mutex
uint8_t receive_state = 0;
uint8_t control_state = 0;  // 0: stop 1: run

boolean heartbeat = false;

// Button
const int buttonPin = 32;
const uint8_t ledPin = 26;
const uint8_t ledNum = 48;
CRGB leds[ledNum];

void setup() {
  // M5.begin(true, false, true);
  Serial.begin(115200);

#ifdef USE_SOFT_AP
  init_soft_ap();
#else
  init_wifi_client();
#endif

  pinMode(buttonPin, INPUT_PULLUP);
  FastLED.addLeds<NEOPIXEL, ledPin>(leds, ledNum);
  FastLED.setBrightness(100);
  FastLED.clear();

  delay(500);  // TODO: task起動待ち。せまふぉ？
}

void init_soft_ap() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);

  IPAddress myIP = WiFi.softAPIP();

  Serial.println("WiFi AP OK");
  Serial.print("IP address: ");
  Serial.println(myIP);
  udp.begin(myIP, UDP_PORT);
  delay(1000);
}

void button_led_all(CRGB c) {
  for (int i = 0; i < ledNum; i++) leds[i] = c;
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

  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (blink) {
      // M5.dis.drawpix(0, 0x0000ff);
      button_led_all(CRGB(0, 0, 255));
    } else {
      // M5.dis.drawpix(0, 0x000000);
      button_led_all(CRGB(0, 0, 0));
    }
    blink = !blink;
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // M5.update();
  receiveFromCar();

  control_state = 0;
  // if (M5.BtnA.isPressed() || !button_data) {
  // if (M5.Btn.isPressed() || !digitalRead(buttonPin)) {
  if (!digitalRead(buttonPin)) {
    control_state = 1;  // Run
    // M5.dis.drawpix(0, 0xff0000);  // GRB Green
    button_led_all(CRGB(0, 255, 0));
  } else {
    // M5.dis.drawpix(0, 0x00ff00);  // GRB Red
    button_led_all(CRGB(255, 0, 0));
  }
  // Serial.print("control_state: ");
  // Serial.println(control_state);

  sendControlState(clientIP);
  sendControlState(clientIP2);
  delay(100);  // TODO: remove
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
