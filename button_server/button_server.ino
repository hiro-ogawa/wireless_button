// #define USE_STACK
#define USE_MATRIX

#ifdef USE_STACK
#include <M5Stack.h>
#endif
#ifdef USE_MATRIX
#include <M5Atom.h>
#endif
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

// #define USE_SOFT_AP
const IPAddress ip(192, 168, 88, 1);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress clientIP(192, 168, 88, 2);

const int UDP_PORT = 55555;

WiFiUDP udp;

TaskHandle_t task_handl;

// TODO: mutex
uint8_t receive_state = 0;
uint8_t control_state = 0;  // 0: stop 1: run

boolean heartbeat = false;

const int buttonPin = 32;  // the number of the pushbutton pin

//******** core 0 task *************
void taskController(void *params) {
  while (true) {
    M5.update();

    if (heartbeat) {
#ifdef USE_STACK
      M5.Lcd.fillRect(0, 0, 50, 50, TFT_RED);  // TODO: heart icon
#endif
#ifdef USE_MATRIX
      M5.dis.drawpix(2, 2, 0xff0000);
#endif
      heartbeat = false;
    } else {
#ifdef USE_STACK
      M5.Lcd.fillRect(0, 0, 50, 50, TFT_BLACK);  // TODO: heart icon
#endif
#ifdef USE_MATRIX
      M5.dis.drawpix(2, 2, 0x00ff00);
#endif
    }

    // TODO: rendar state

    bool btn_is_pressed = false;
#ifdef USE_STACK
    btn_is_pressed = M5.BtnA.isPressed();
#endif
#ifdef USE_MATRIX
    btn_is_pressed = M5.Btn.isPressed();
    btn_is_pressed = !digitalRead(buttonPin);
#endif
    if (btn_is_pressed) {
      control_state = 1;  // run
    } else {
      control_state = 0;  // stop
    }
    delay(10);
  }
}

//********* core 1 task ************
void setup() {
  M5.begin(true, false, true);

#ifdef USE_STACK
  M5.Lcd.fillScreen(TFT_BLACK);
#endif

  Serial.begin(115200);

  setupServer();
  xTaskCreatePinnedToCore(&taskController, "taskController", 8192, NULL, 10,
                          &task_handl, 0);

  pinMode(buttonPin, INPUT_PULLUP);

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

void setupServer() {
#ifdef USE_SOFT_AP
  init_soft_ap();
#else
  init_wifi_client();
#endif
}

void loop() {
  receiveFromCar();
  sendControlState();
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

void sendControlState() {
  udp.beginPacket(clientIP, UDP_PORT);
  udp.write(control_state);
  udp.endPacket();
}