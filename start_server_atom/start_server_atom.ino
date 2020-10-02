#include <M5Atom.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"

#define USE_SOFT_AP
const IPAddress ip(192, 168, 88, 1);
// const IPAddress ip(192, 168, 88, 2);
const IPAddress gateway(192, 168, 88, 254);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 88, 254);

const IPAddress clientIP1(192, 168, 88, 3);
const IPAddress clientIP2(192, 168, 88, 4);

const int UDP_PORT = 55555;

WiFiUDP udp;

uint8_t receive_control_state = 0;  // 0: stop 1: run
uint8_t running_state = 0;

SoftwareSerial GroveSerial(26, 32);  // RX, TX

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

  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (blink) {
      M5.dis.drawpix(0, 0x0000ff);
    } else {
      M5.dis.drawpix(0, 0x000000);
    }
    blink = !blink;
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  M5.begin(true, false, true);
  delay(50);
  M5.dis.drawpix(0, 0xffffff);

  Serial.begin(115200);
  GroveSerial.begin(9600);

#ifdef USE_SOFT_AP
  init_soft_ap();
#else
  init_wifi_client();
#endif

  delay(1000);  // FIXME: remove
}

void loop() {
  M5.update();

  receive_control_state = 255;

  int packetSize = udp.parsePacket();
  Serial.println(packetSize);
  if (packetSize) {
    receive_control_state = udp.read();
  }

  char SerialCmd;
  if (M5.Btn.wasReleased())
    receive_control_state = 0;
  else if (M5.Btn.isPressed())
    receive_control_state = 1;

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

  if (receive_control_state == 0 || receive_control_state == 255) {
    M5.dis.drawpix(0, 0x00ff00);  // GRB
  } else {
    M5.dis.drawpix(0, 0xff0000);  // GRB
  }

  if (receive_control_state < 255) {
    Serial.println(SerialCmd);
    GroveSerial.println(SerialCmd);
  } else
    Serial.println("No data");

  delay(100);
}