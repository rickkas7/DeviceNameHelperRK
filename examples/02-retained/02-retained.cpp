#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

retained DeviceNameHelperData deviceNameHelperRetained;

void setup() {
    // This two lines are here so you can see the debug logs. You probably
    // don't want them in your code.
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    DeviceNameHelperRetained::instance().withNameCallback([](const char *name) {
        Log.info("name=%s", name);
    });

    // You must call this from setup!
    DeviceNameHelperRetained::instance().setup(&deviceNameHelperRetained);
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperRetained::instance().loop();
}
