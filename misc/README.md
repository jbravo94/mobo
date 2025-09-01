.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/mbedtls

#include <ESP_IOExpander_Library.h>

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


bool isSnoozed = false;


int last_day = 0;

bool isSnoozeOver () {

  if (!isSnoozed) {
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

  return isSnoozeOver;

  return false;
}