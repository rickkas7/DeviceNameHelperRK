#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

int EEPROM_OFFSET = 1;

void setup() {
    // This two lines are here so you can see the debug logs. You probably
    // don't want them in your code.
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    // You must call this from setup!
    DeviceNameHelperEEPROM::instance().setup(EEPROM_OFFSET);
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperEEPROM::instance().loop();

    // This is just for displaying the name. You wouldn't have this in your code.
    // See 02-retained.cpp for an easier way to do this using a C++11 lambda
    // instead of adding code to loop.
    static bool reported = false;
    if (!reported && DeviceNameHelperEEPROM::instance().hasName()) {
        reported = true;
        Log.info("name=%s", DeviceNameHelperEEPROM::instance().getName());
    }
}
