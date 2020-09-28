#include <M5Atom.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

const IPAddress ip(192, 168, 88, 2);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress serverIP(192, 168, 88, 1);

const int UDP_PORT = 55555;

WiFiUDP udp;

TaskHandle_t taskStateObserver_handl;
TaskHandle_t taskSendRunningState_handl;

// TODO: mutex
SemaphoreHandle_t receive_control_state_mu = NULL;
uint8_t receive_control_state = 0;  // 0: stop 1: run
uint8_t running_state = 0;

SoftwareSerial GroveSerial(26, 32);  // RX, TX

void taskStateObserver(void *params) {
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

  udp.begin(WiFi.localIP(), UDP_PORT);

  delay(1000);

  xTaskCreatePinnedToCore(&taskSendRunningState, "taskSendRunningState", 8192,
                          NULL, 10, &taskSendRunningState_handl, 0);

  BaseType_t xStatus;
  xSemaphoreGive(receive_control_state_mu);

  while (true) {
    M5.update();

    xStatus = xSemaphoreTake(receive_control_state_mu, 500UL);
    if (xStatus == pdTRUE) {
      char SerialCmd;
      if (M5.Btn.isPressed()) receive_control_state = 1;
      switch (receive_control_state) {
        case 0:
          // Stop
          SerialCmd = 'S';
          break;
        case 1:
          // Run
          SerialCmd = 'G';
          break;
        case 2:
          // Left
          SerialCmd = 'H';
          break;
        case 3:
          // Down
          SerialCmd = 'J';
          break;
        case 4:
          // Up
          SerialCmd = 'K';
          break;
        case 5:
          // Right
          SerialCmd = 'L';
          break;
      }
      if (receive_control_state == 0) {
        M5.dis.drawpix(0, 0x00ff00);  // GRB
      } else {
        M5.dis.drawpix(0, 0xff0000);  // GRB
        receive_control_state = 0;
      }
      Serial.println(SerialCmd);
      GroveSerial.println(SerialCmd);
    }
    xSemaphoreGive(receive_control_state_mu);

    delay(300);
  }
}

void setup() {
  M5.begin(true, false, true);
  delay(50);
  M5.dis.drawpix(0, 0xffffff);

  Serial.begin(115200);
  GroveSerial.begin(9600);

  receive_control_state_mu = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(&taskStateObserver, "taskStateObserver", 8192, NULL,
                          1, &taskStateObserver_handl, 1);
  delay(1000);  // FIXME: remove
}

void taskSendRunningState(void *params) {
  BaseType_t xStatus;
  xSemaphoreGive(receive_control_state_mu);

  while (true) {
    udp.beginPacket(serverIP, UDP_PORT);
    udp.write(running_state);
    udp.endPacket();

    xStatus = xSemaphoreTake(receive_control_state_mu, 500UL);
    if (xStatus == pdTRUE) {
      int packetSize = udp.parsePacket();
      if (packetSize > 0) {
        receive_control_state = udp.read();
      }
    }
    xSemaphoreGive(receive_control_state_mu);
  }
}

void loop() {}