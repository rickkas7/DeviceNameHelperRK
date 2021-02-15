#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

void setup() {
    // This two lines are here so you can see the debug logs. You probably
    // don't want them in your code.
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    // This causes the name to be fetched every 2 minutes. 
    // This is only for demonstration purposes, you probably wouldn't want 
    // to fetch it that often!
    // A more common value would be 24h, or once a day.
    DeviceNameHelperNoStorage::instance().withCheckPeriod(2min);

    // This just prints the name when we have it. You don't need this
    // in your code.
    DeviceNameHelperNoStorage::instance().withNameCallback([](const char *name) {
        Log.info("name=%s", name);
    });

    // You must call this from setup!
    DeviceNameHelperNoStorage::instance().setup();
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperNoStorage::instance().loop();
}
