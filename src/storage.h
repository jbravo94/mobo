#include <Arduino.h>

void setup_preferences();
void setup_littlefs();
String getStringFromPreferences(const char* key, String defaultValue);
void putStringFromPreferences(const char* key, String value);
