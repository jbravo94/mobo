#include "storage.h"
#include "LittleFS.h"
#include <Preferences.h>
#include "serial.h"

Preferences preferences;

void setup_littlefs() {
   if (!LittleFS.begin(true)) {
    LOGLN("An error has occurred while mounting LittleFS");
  }
  LOGLN("LittleFS mounted successfully");
}

void setup_preferences() {
  preferences.begin("backup-buddy", false); 
}

String getStringFromPreferences(const char *key, String defaultValue) {
    return preferences.getString(key, defaultValue);
}

void putStringFromPreferences(const char *key, String value) {
    preferences.putString(key, value);
}
