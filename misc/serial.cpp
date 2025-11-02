#include "serial.h"
#include "HWCDC.h"

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

bool isDebugEnabled() {
    return debug;
}

void setup_serial() {
  USBSerial.begin(115200);
  LOGLN("Serial initialized.");
}
