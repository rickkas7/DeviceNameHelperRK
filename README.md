# DeviceNameHelperRK

*Library for accessing the Particle device name easily*

The device name for a Particle device is stored in the cloud, not locally on the device. While you can retrieve the name from code running on the device, the process can necessarily only be done while connected to the cloud, and it requires time and data. 

This library provides an easy-to-use wrapper and also provides a way to store the data locally:

- `DeviceNameHelperEEPROM` stores the name in the EEPROM emulation. It is preserved across power-down, sleep, reboot, user code flash, and Device OS flash. The EEPROM is typically around 3K. The library requires 44 bytes of retained memory for the name and some other data.
- `DeviceNameHelperRetained` stores the name in retained memory. It is preserved across sleep modes and reboot. The retained memory is around 3K on most devices. The library requires 44 bytes of retained memory for the name and some other data.
- `DeviceNameHelperFile` stores the name in a file on the flash file system. This requires a Gen 3 device (Argon, Boron, B Series SoM, or Tracker SoM) and Device OS 2.0.0 or later. The flash file system is 2MB (4 MB on the Tracker).
- `DeviceNameHelperNoStorage` stores the name in RAM so it will be fetched on every restart and also after HIBERNATE sleep.

Also:

- The full browsable API documentation can be found in the docs/html folder and [here](https://rickkas7.github.io/DeviceNameHelperRK/index.html).
- The Github repository for this project is: [https://github.com/rickkas7/DeviceNameHelperRK](https://github.com/rickkas7/DeviceNameHelperRK).
- The license is MIT. Can be used in commercial, open-source, and closed-source products. Attribution not required.


## Examples

### Overview

All of the classes use a singleton pattern. You use the `instance()` method of your class to allocate it, if it doesn't exist, or retrieve the existing instance. For example: `DeviceNameHelperEEPROM::instance()`. There is also `DeviceNameHelperRetained::instance()`, etc.

There is one method you must call from setup(), and one from your app's loop(). 

For example:

```
void setup() {
    // You must call this from setup!
    DeviceNameHelperEEPROM::instance().setup(EEPROM_OFFSET);
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperEEPROM::instance().loop();
}
```

Replace `DeviceNameHelperEEPROM` with whatever storage method you are using such as `DeviceNameHelperRetained` or `DeviceNameHelperFile`.

To use the name, call `DeviceNameHelperEEPROM::instance().getName()`. This returns a c-string containing the name. The name is limited to `DEVICENAMEHELPER_MAX_NAME_LEN` characters (31). 

To find out if the name has been retrieved, use `DeviceNameHelperEEPROM::instance().hasName()`.



### EEPROM

`DeviceNameHelperEEPROM` stores the name in the EEPROM emulation. It is preserved across power-down, sleep, reboot, user code flash, and Device OS flash. The EEPROM is typically around 3K. The library requires 44 bytes of retained memory for the name and some other data.

To store the data in EEPROM emulation you must select a starting offset (0 or greater) with unused space of 44 bytes, `sizeof(DeviceNameHelperData)`.

```cpp
#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

int EEPROM_OFFSET = 100;

void setup() {
    // You must call this from setup!
    DeviceNameHelperEEPROM::instance().setup(EEPROM_OFFSET);
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperEEPROM::instance().loop();
}
```

### Retained

`DeviceNameHelperRetained` stores the name in retained memory. It is preserved across sleep modes and reboot. The retained memory is around 3K on most devices. The library requires 44 bytes of retained memory for the name and some other data.

```cpp
#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

retained DeviceNameHelperData deviceNameHelperRetained;

void setup() {
    // You must call this from setup!
    DeviceNameHelperRetained::instance().setup(&deviceNameHelperRetained);
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperRetained::instance().loop();
}
```

### File

`DeviceNameHelperFile` stores the name in a file on the flash file system. This requires a Gen 3 device (Argon, Boron, B Series SoM, or Tracker SoM) and Device OS 2.0.0 or later. The flash file system is 2MB (4 MB on the Tracker).

On Gen 3 devices, EEPROM is implemented as a file on the flash file system, so there is no real advantage of using EEPROM over File on Gen 3 devices running Device OS 2.0.0 or later.

```cpp
#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

void setup() {
    // You must call this from setup!
    DeviceNameHelperFile::instance().setup();
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperFile::instance().loop();
}
```

You can optionally pass a filename to setup(). The default filename is /usr/devicename.

```cpp
void setup() {
    // You must call this from setup!
    DeviceNameHelperFile::instance().setup("/usr/mydevicename.txt");
}
```

### No storage

`DeviceNameHelperNoStorage` stores the name in RAM so it will be fetched on every restart. 

HIBERNATE sleep mode also clears RAM, so if you use HIBERNATE, the device name will have to be fetched from the cloud on every wake, which is not particularly efficient. You will probably want to use a different method in this scenario.

```cpp
#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

void setup() {
    // You must call this from setup!
    DeviceNameHelperNoStorage::instance().setup();
}

void loop() {
    // You must call this from loop!
    DeviceNameHelperNoStorage::instance().loop();
}
```

### Name Callback

Sometimes you may want to know when the name is available. The name callback can be used for this purpose. If the name was stored (EEPROM, retained, or file) then the name will be called during setup(). Otherwise, the name callback will be called after the name has been retrieved from the cloud.

```cpp
#include "DeviceNameHelperRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

retained DeviceNameHelperData deviceNameHelperRetained;

void nameCallback(const char *name) {
    Log.info("name=%s", name);
}

void setup() {
    DeviceNameHelperRetained::instance().withNameCallback(nameCallback);
    DeviceNameHelperRetained::instance().setup(&deviceNameHelperRetained);
}

void loop() {
    DeviceNameHelperRetained::instance().loop();
}

```

The name callback can also be a C++11 lambda. A lambda can also be used to call a C++ member function. Note the body of the lambda is called later, when the name is known; the withNameCallback() function does not block.

```cpp
void setup() {
    DeviceNameHelperRetained::instance().withNameCallback([](const char *name) {
        Log.info("name=%s", name);
    });

    DeviceNameHelperRetained::instance().setup(&deviceNameHelperRetained);
}
```

### Check Period

By default, the name is only checked once. If you later change the name, the name will not be retrieved again unless the name is no longer available from the storage method, such as after powering down completely while using retained memory.

To periodically fetch the name, use the `withCheckPeriod()` method. For example:

```cpp
void setup() {
    // This causes the name to be fetched once per day
    DeviceNameHelperNoStorage::instance().withCheckPeriod(24h);

    // You must call this from setup!
    DeviceNameHelperNoStorage::instance().setup();
}
```

The parameter is a chrono literal. Common units include `h` for hours and `min` for minutes.

## Version History

### 0.0.1 (2021-02-15)

- Initial version
