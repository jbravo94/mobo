#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "aws_client.h"
#include "serial.h"
#include "storage.h"
#include "web_server.h"
#include "pin_config.h"

int http_delay = 5000;
int loop_delay = 100;

int loop_delay_counter = 0;

#include <ESP_IOExpander_Library.h>
#define _EXAMPLE_CHIP_CLASS(name, ...) ESP_IOExpander_##name(__VA_ARGS__)
#define EXAMPLE_CHIP_CLASS(name, ...) _EXAMPLE_CHIP_CLASS(name, ##__VA_ARGS__)

ESP_IOExpander *expander = NULL;


void setup_button() {
  
  expander = new EXAMPLE_CHIP_CLASS(TCA95xx_8bit,
                                    (i2c_port_t)0, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
                                    IIC_SCL, IIC_SDA);

  expander->init();
  expander->begin();
  expander->pinMode(5, INPUT);
  expander->pinMode(4, INPUT);
}

void loop_button() {
  int backlight_ctrl = expander->digitalRead(4);

  if (backlight_ctrl == HIGH) {
    while (expander->digitalRead(4) == HIGH) {
      delay(50);
    }
    toggle_backlight();
  }
}


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

  setup_button();

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

  loop_button();

  loop_delay_counter += 1;
  delay(100);
}

