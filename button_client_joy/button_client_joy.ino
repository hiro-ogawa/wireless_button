#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include "config.h"

const IPAddress ip(192, 168, 88, 4);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress serverIP1(192, 168, 88, 1);
const IPAddress serverIP2(192, 168, 88, 2);

const int UDP_PORT = 55555;

WiFiUDP udp;

uint8_t receive_state = 0;
uint8_t control_state = 0;  // 0: stop 1: run

bool heartbeat = false;

// Joy
#define FACE_JOY_ADDR 0x5e
uint8_t x_data_L;
uint8_t x_data_H;
uint16_t x_data;
uint8_t y_data_L;
uint8_t y_data_H;
uint16_t y_data;
uint8_t button_data;
char data[100];

void setup() {
  M5.begin();

  Serial.begin(115200);

  JoyInit();

  init_wifi_client();

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.clear();
  M5.Lcd.setCursor(60, 0, 4);
  M5.Lcd.printf("FACE JOYSTICK");

  delay(500);  // TODO: task起動待ち。せまふぉ？
}

void init_wifi_client() {
  WiFi.config(ip, gateway, subnet, dns);
  delay(10);

  // We start by connecting to a WiFi network -----------------------------
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.clear();
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    M5.Lcd.setCursor(60, 0, 4);
    M5.Lcd.printf("Connecting count: %d", count++);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  M5.update();
  JoyDisplay();
  receiveFromCar();

  bool btn_is_pressed = false;
  btn_is_pressed = M5.BtnA.isPressed();
  control_state = 0;
  // if (M5.BtnA.isPressed() || !button_data) {
  if (M5.BtnA.isPressed()) {
    control_state = 1;  // Run
  } else if (x_data < 250) {
    control_state = 2;  // Left
  } else if (x_data > 750) {
    control_state = 5;  // Right
  } else if (y_data < 250) {
    control_state = 3;  // Down
  } else if (y_data > 750) {
    control_state = 4;  // Up
  }
  Serial.print("control_state: ");
  Serial.println(control_state);

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

void JoyInit() {
  Wire.begin();
  for (int i = 0; i < 256; i++) {
    Wire.beginTransmission(FACE_JOY_ADDR);
    Wire.write(i % 4);
    Wire.write(random(256) * (256 - i) / 256);
    Wire.write(random(256) * (256 - i) / 256);
    Wire.write(random(256) * (256 - i) / 256);
    Wire.endTransmission();
    delay(2);
  }
  Led(0, 0, 0, 0);
  Led(1, 0, 0, 0);
  Led(2, 0, 0, 0);
  Led(3, 0, 0, 0);
}

void Led(int indexOfLED, int r, int g, int b) {
  Wire.beginTransmission(FACE_JOY_ADDR);
  Wire.write(indexOfLED);
  Wire.write(r);
  Wire.write(g);
  Wire.write(b);
  Wire.endTransmission();
}

void JoyDisplay() {
  Wire.requestFrom(FACE_JOY_ADDR, 5);
  if (Wire.available()) {
    y_data_L = Wire.read();
    y_data_H = Wire.read();
    x_data_L = Wire.read();
    x_data_H = Wire.read();

    button_data = Wire.read();  // Z(0: released 1: pressed)

    x_data = x_data_H << 8 | x_data_L;
    y_data = y_data_H << 8 | y_data_L;

    sprintf(data, "x:%d y:%d button:%d\n", x_data, y_data, button_data);
    Serial.print(data);

    M5.Lcd.setCursor(45, 120);
    M5.Lcd.println(data);

    if (x_data > 600) {
      Led(2, 0, 0, 50);
      Led(0, 0, 0, 0);
    } else if (x_data < 400) {
      Led(0, 0, 0, 50);
      Led(2, 0, 0, 0);
    } else {
      Led(0, 0, 0, 0);
      Led(2, 0, 0, 0);
    }

    if (y_data > 600) {
      Led(3, 0, 0, 50);
      Led(1, 0, 0, 0);
    } else if (y_data < 400) {
      Led(1, 0, 0, 50);
      Led(3, 0, 0, 0);
    } else {
      Led(1, 0, 0, 0);
      Led(3, 0, 0, 0);
    }
  }
}
