#include "serial.h"
#include "HWCDC.h"
#include "sys/time.h"
#include "time.h"
#include "storage.h"

HWCDC USBSerial;
bool debug = true;

#undef LOG
#undef LOGLN
#undef LOGF
#undef WRITE

#define LOG(text) { if (debug) { USBSerial.print(text); } }
#define LOGLN(text) { if (debug) { USBSerial.println(text); } }
#define LOGF(text, ...) { if (debug) { USBSerial.printf(text, ##__VA_ARGS__); } }
#define WRITE(text) { USBSerial.write(text); } }



String receivedMessage = "";
bool isLoggedIn = false;
uint32_t loggedInMillisTimestamp;

int logoutTimeout = 30;

String adminPassword = "BackupBuddyAdmin";

void llogln(String text) {
  USBSerial.println(text);
}

bool isDebugEnabled() {
    return debug;
}

void setup_serial() {
  USBSerial.begin(115200);
  LOGLN("Serial initialized.");
}

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
        LOGLN(getStringFromPreferences(value.c_str(), ""));
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
        putStringFromPreferences(key.c_str(), val.c_str());

        /*
        preferences.putBytes(key.c_str(), val.c_str(), (size_t) value.length());

        size_t valueLen = preferences.getBytesLength(key.c_str());
        char buffer[valueLen];


        preferences.getBytes(key.c_str(), &buffer, valueLen);
        LOGLN(buffer);
        */

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
