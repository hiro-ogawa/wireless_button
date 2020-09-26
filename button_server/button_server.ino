#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

const IPAddress ip(192, 168, 88, 1);
const IPAddress subnet(255, 255, 255, 0);

const IPAddress clientIP(192, 168, 88, 2);

const int UDP_PORT = 55555;

WiFiUDP udp;

TaskHandle_t task_handl;

// TODO: mutex
uint8_t receive_state = 0;
uint8_t control_state = 0;  // 0: stop 1: run

boolean heartbeat = false;

//******** core 0 task *************
void taskController(void *params) {
  while (true) {
    M5.update();

    if (heartbeat) {
      M5.Lcd.fillRect(0, 0, 50, 50, TFT_RED);  // TODO: heart icon
      heartbeat = false;
    } else {
      M5.Lcd.fillRect(0, 0, 50, 50, TFT_BLACK);  // TODO: heart icon
    }

    // TODO: rendar state

    if (M5.BtnA.isPressed()) {
      control_state = 1;  // run
    } else {
      control_state = 0;  // stop
    }
    delay(10);
  }
}

//********* core 1 task ************
void setup() {
  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);

  Serial.begin(115200);

  setupServer();
  xTaskCreatePinnedToCore(&taskController, "taskController", 8192, NULL, 10,
                          &task_handl, 0);

  delay(500);  // TODO: task起動待ち。せまふぉ？
}

void setupServer() {
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