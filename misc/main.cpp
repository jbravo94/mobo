#include <Arduino.h>

#include <WiFi.h>

#include <Preferences.h>
#include "LittleFS.h"

#include "display.h"
#include "aws_client.h"
#include "serial.h"

//#include <ESP_IOExpander_Library.h>

// Login session

bool isLoggedIn = false;
uint32_t loggedInMillisTimestamp;

int logoutTimeout = 30;

String adminPassword = "BackupBuddyAdmin";

Preferences preferences;


int http_delay = 5000;
int loop_delay = 100;

int loop_delay_counter = 0;



bool isSnoozed = false;
int last_day = 0;


// USB Serial


String receivedMessage = "";




bool isSnoozeOver () {

  /*if (!isSnoozed) {
    return false;
  }

  struct tm timeinfo = getTimeInfo();

  if ((last_day == 365 || last_day == 364) && timeinfo.tm_yday == 0) {
    isSnoozed = false;
    return true;
  }

  //bool isSnoozeOver = timeinfo.tm_yday > last_day && timeinfo.tm_hour > (8 - 2);

  bool isSnoozeOver = timeinfo.tm_sec > ((last_day + 10) % 60);

  if (isSnoozeOver) {
    isSnoozed = false;
  }

  last_day = timeinfo.tm_sec;

  //last_day = timeinfo.tm_yday;

  return isSnoozeOver;*/

  return false;
}


void setup_preferences() {
  preferences.begin("backup-buddy", false); 
}

void setup_wifi() {

  String ssid = preferences.getString("wifi-ssid", "backup-buddy-wifi-ssid");
  String password = preferences.getString("wifi-password", "backup-buddy-wifi-password");

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
  LOGLN(WiFi.localIP());
}

void setup_littlefs() {
   if (!LittleFS.begin(true)) {
    LOGLN("An error has occurred while mounting LittleFS");
  }
  LOGLN("LittleFS mounted successfully");
}


void setup(void) {
  setup_preferences();
  setup_serial();

  setup_wifi();

  setup_time();

  bootstrap_wire_touchpad_display();

  
  setup_httpclient();

  setup_littlefs();

  //setup_button();

  delay(2000);  // 5 seconds
}

void handle_http_response() {
  if (areBackupsSuccessful()) {
    set_circle_green();

    if (isSnoozeOver() && !is_backlight_on()) {
      toggle_backlight();
    }
  } else {
    set_circle_red();

    isSnoozed = false;

    if (!is_backlight_on()) {
      toggle_backlight();
    }
  }

   /*if (backupsSuccessful) {
    set_display_rectangle(200, GREEN);
  } else {
    set_display_rectangle(200, RED);
  }*/
}


/*

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
    toggleBacklight();
  }
}
*/

/*
void loop_serial() {
  while (USBSerial.available()) {
    char incomingChar = USBSerial.read();  // Read each character from the buffer
    
    if (incomingChar == '\n') {  // Check if the user pressed Enter (new line character)

      LOG("You sent: ");
      LOGLN(receivedMessage);

      String auth_command = "AUTH";
      String get_command = "GET";
      String put_command = "PUT";
      String quit_command = "QUIT";

      if (isLoggedIn && quit_command.equals(receivedMessage)) {

        isLoggedIn = false;
        loggedInMillisTimestamp = 0;

        LOGLN("Successfully logged out.");
        receivedMessage = "";
        return;
      }

      int index = receivedMessage.indexOf(' ');
      if (index == -1) {
        receivedMessage = "";
        LOGLN("Unknown command.");
        return;
      }

      String command = receivedMessage.substring(0, index);
      String value = receivedMessage.substring(index + 1);

      if (auth_command.equals(command) && adminPassword.equals(value)) {
        isLoggedIn = true;
        loggedInMillisTimestamp = millis();
        LOGLN("Login successful.");
        receivedMessage = "";
        return;
      }

      if ((millis() - loggedInMillisTimestamp) >= (logoutTimeout * 1000)) {
        isLoggedIn = false;
      }

      if (!isLoggedIn) {
        LOGLN("Unauthorized.");
        receivedMessage = "";
        return;
      }

      if (get_command.equals(command)) {
        LOG("Value: ");
        LOGLN(preferences.getString(value.c_str(), ""));
        receivedMessage = "";
      }

      if (put_command.equals(command)) {

        int idx = value.indexOf(' ');

        if (idx == -1) {
          receivedMessage = "";
          LOGLN("Syntax incorrect. Use 'PUT key value");
          return;
        }

        String key = value.substring(0, idx);
        String val = value.substring(idx + 1);

        LOGF("Key %s, Val %s ", key, val);
        preferences.putString(key.c_str(), val.c_str());

        preferences.putBytes(key.c_str(), val.c_str(), (size_t) value.length());

        size_t valueLen = preferences.getBytesLength(key.c_str());
        char buffer[valueLen];


        preferences.getBytes(key.c_str(), &buffer, valueLen);
        LOGLN(buffer);

        receivedMessage = "";
      }
            
      // Clear the message buffer for the next input
      receivedMessage = "";
    } else {
      // Append the character to the message string
      receivedMessage += incomingChar;
    }
  }
}
*/

void loop() {
  //loop_beacon();

  if (http_delay <= (loop_delay_counter * loop_delay)) {
    loop_httpclient();
    handle_http_response();
    loop_delay_counter = 0;
  }


  refresh_ui();
  //loop_serial();

  loop_delay_counter += 1;
  delay(100);
}

