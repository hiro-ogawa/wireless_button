#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

const IPAddress serverIP(192, 168, 88, 1);

const int UDP_PORT = 55555;

WiFiUDP udp;

TaskHandle_t taskStateObserver_handl;
TaskHandle_t taskSendRunningState_handl;

// TODO: mutex
SemaphoreHandle_t receive_control_state_mu = NULL;
uint8_t receive_control_state = 0;  // 0: stop 1: run
uint8_t running_state = 0;

void taskStateObserver(void *params) {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
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
      if (receive_control_state == 1) {
        // running
        //      M5.Lcd.fillRect(0, 0, 100, 50, TFT_BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.println("run!!!");
        receive_control_state = 0;
      } else {
        M5.Lcd.fillRect(0, 0, 100, 50, TFT_BLACK);
        // TODO: stop
      }
    }
    xSemaphoreGive(receive_control_state_mu);

    delay(100);
  }
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);

  M5.Lcd.setTextSize(2);

  Serial.begin(115200);

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