#include <Arduino.h>
#include <NimBLEDevice.h>

#define BLE_SCAN_INTERVAL 0x80
#define BLE_SCAN_WINDOW 0x80
#define SCAN_TASK_STACK_SIZE 2562

unsigned int totalFpQueried = 0;
TaskHandle_t scanTaskHandle;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice *advertisedDevice) {

        //bleStack = uxTaskGetStackHighWaterMark(nullptr);
        //BleFingerprintCollection::Seen(advertisedDevice);
        Serial.println(String(advertisedDevice->getAddress()));
    }
};

void scanTask(void *parameter) {
    NimBLEDevice::init("ESPresense");

    NimBLEDevice::setMTU(23);

    auto pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setInterval(BLE_SCAN_INTERVAL);
    pBLEScan->setWindow(BLE_SCAN_WINDOW);
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(false);
    pBLEScan->setDuplicateFilter(false);
    pBLEScan->setMaxResults(0);
    if (!pBLEScan->start(0, nullptr, false))
        log_e("Error starting continuous ble scan");

    while (true) {
        //for (auto &f : BleFingerprintCollection::fingerprints)
        //    if (f->query())
        //        totalFpQueried++;

        if (!pBLEScan->isScanning()) {
            if (!pBLEScan->start(0, nullptr, true))
                log_e("Error re-starting continuous ble scan");
            delay(3000);  // If we stopped scanning, don't query for 3 seconds in order for us to catch any missed broadcasts
        } else {
            delay(100);
        }
    }
}

void setup() { 
  Serial.begin(115200);
  xTaskCreatePinnedToCore(scanTask, "scanTask", SCAN_TASK_STACK_SIZE, nullptr, 1, &scanTaskHandle, CONFIG_BT_NIMBLE_PINNED_TO_CORE);
}

void loop() {
  Serial.println("TEST");
  delay(1000);
}