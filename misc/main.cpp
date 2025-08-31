#include <Arduino.h>
#include "HWCDC.h"
#include "LittleFS.h"
#include <WiFiClientSecure.h>


HWCDC USBSerial;

void setup_littlefs() {
   if (!LittleFS.begin(true)) {
    USBSerial.println("An error has occurred while mounting LittleFS");
  }
  USBSerial.println("LittleFS mounted successfully");
}

void setup() { 
   USBSerial.begin(115200);
   setup_littlefs();
}

void loop() {
  USBSerial.println("TEST");
  delay(1000);
}
