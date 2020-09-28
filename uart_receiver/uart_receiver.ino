#include <M5Core2.h>
#include <SoftwareSerial.h>

SoftwareSerial GroveSerial;

void setup() {
  M5.begin();
  Serial.begin(115200);
  GroveSerial.begin(9600, SWSERIAL_8N1, 33);
}

void loop() {
  while (GroveSerial.available()) {
    char c = GroveSerial.read();
    if (c == 10 || c == 13) break;

    Serial.println(c);
    switch (c) {
      case 'S':
        M5.Lcd.fillScreen(RED);
        break;
      default:
        M5.Lcd.fillScreen(GREEN);
        break;
    }
    M5.Lcd.setCursor(150, 110);
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.setTextSize(7);
    M5.Lcd.printf("%c", c);
  }
}