#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "aws_client.h"
#include "serial.h"
#include "storage.h"
#include "web_server.h"

int http_delay = 5000;
int loop_delay = 100;

int loop_delay_counter = 0;


void setup_wifi() {

  String ssid = getStringFromPreferences("wifi-ssid", "backup-buddy-wifi-ssid");
  String password = getStringFromPreferences("wifi-password", "backup-buddy-wifi-password");

  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(ssid, password);

  int retry_timeout = 5000;
  int retry_counter = 0;

  LOGLN("Connecting to Wifi.");

  while (WiFi.status() != WL_CONNECTED) {
    LOG(".");

    if ((100 * retry_counter) >= retry_timeout) {
      break;
    }

    retry_counter += 1;
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    LOGLN("\nFailed to connect to network...");
    return;
  }

  LOGLN("\nConnected to the WiFi network.");
  LOG("Local IP: ");
  llogln(WiFi.localIP().toString());
  LOGLN(WiFi.localIP());
}



void setup(void) {
  setup_preferences();
  setup_serial();

  setup_wifi();

  setup_time();

  bootstrap_wire_touchpad_display();

  
  setup_httpclient();

  setup_littlefs();

  setup_web_server();

  delay(2000);
}


void handle_http_response() {
  if (areBackupsSuccessful()) {
    set_circle_green();

    if (!is_backlight_on()) {
      toggle_backlight();
    }
  } else {
    set_circle_red();

    if (!is_backlight_on()) {
      toggle_backlight();
    }
  }
}


void loop() {

  loop_web_server();

  if (http_delay <= (loop_delay_counter * loop_delay)) {
    loop_httpclient();
    handle_http_response();
    loop_delay_counter = 0;
  }

  refresh_ui();
  loop_serial();

  loop_delay_counter += 1;
  delay(100);
}

