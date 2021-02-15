#include "DeviceNameHelperRK.h"

DeviceNameHelper *DeviceNameHelper::_instance = 0;

void DeviceNameHelper::loop() {
    if (stateHandler) {
        stateHandler(*this);
    }   
}

DeviceNameHelper &DeviceNameHelper::withNameCallback(std::function<void(const char *)> nameCallback) {
    this->nameCallback = nameCallback;
    return *this;
}


DeviceNameHelper::DeviceNameHelper() {
}

DeviceNameHelper::~DeviceNameHelper() {
}

void DeviceNameHelper::commonSetup() {
    // Validate data
    if (data->magic != DATA_MAGIC || data->size != sizeof(DeviceNameHelperData)) {
        memset(data, 0, sizeof(DeviceNameHelperData));     
        data->magic = DATA_MAGIC;
        data->size = (uint8_t) sizeof(DeviceNameHelperData);
    }

    stateHandler = &DeviceNameHelper::stateStart;
}

void DeviceNameHelper::checkName() {
    if (stateHandler == NULL) {
        stateHandler = &DeviceNameHelper::stateSubscribe;
        return;
    }

    forceCheck = true;
}


void DeviceNameHelper::save() {
    // Overridden by DeviceNameHelperEEPROM
}

void DeviceNameHelper::stateStart() {
    if (data->name[0]) {
        // We have a name and we are not rechecking
        if (nameCallback) {
            nameCallback(data->name);
        }
        stateHandler = &DeviceNameHelper::stateWaitRecheck;
        stateTime = millis();
        return;
    }

    // Subscribe
    stateHandler = &DeviceNameHelper::stateSubscribe;
}


void DeviceNameHelper::stateSubscribe() {

    if (!hasSubscribed) {
        // Add a subscription handler for the device name event
        Particle.subscribe("particle/device/name", &DeviceNameHelper::subscriptionHandler, this);
        hasSubscribed = true;
    }

    stateHandler = &DeviceNameHelper::stateWaitConnected;
}

void DeviceNameHelper::stateWaitConnected() {
    if (!Particle.connected() || !Time.isValid()) {
        // Not connected or do not have the time yet
        return;
    }

    stateHandler = &DeviceNameHelper::stateWaitRequest;
    stateTime = millis();
}

void DeviceNameHelper::stateWaitRequest() {
    // Wait a few seconds for the subscription to complete
    if (millis() - stateTime < POST_CONNECT_WAIT_MS) {
        return;
    }
    // Now request device name
    gotResponse = false;
    Particle.publish("particle/device/name");

    stateHandler = &DeviceNameHelper::stateWaitResponse;
    stateTime = millis();
}

void DeviceNameHelper::stateWaitResponse() {
    if (gotResponse) {
        // Got a response
        if (data->name[0]) {
            // And a name
            data->lastCheck = Time.now();
            save();

            if (nameCallback) {
                nameCallback(data->name);
            }

            // Recheck later
            stateHandler = &DeviceNameHelper::stateWaitRecheck;
            stateTime = millis();
            return;
        } else {
            // Got a response but no name. Try again in a few minutes.
            stateHandler = &DeviceNameHelper::stateWaitRetry;
            stateTime = millis();
            return;
        }
    }

    if (millis() - stateTime >= RESPONSE_WAIT_MS) {
        // Did not get a response
        stateHandler = &DeviceNameHelper::stateWaitRetry;
        stateTime = millis();
        return;
    }
}

void DeviceNameHelper::stateWaitRetry() {
    if (millis() - stateTime >= RETRY_WAIT_MS) {
        // Time to retry
        stateHandler = &DeviceNameHelper::stateWaitConnected;
        return;
    }
}

void DeviceNameHelper::stateWaitRecheck() {
    if (millis() - stateTime < 10000) {
        return;
    }
    stateTime = millis();

    // Only do these checks every 10 seconds

    if (forceCheck) {
        forceCheck = false;
        stateHandler = &DeviceNameHelper::stateSubscribe;
        return;
    }

    if (checkPeriod.count() == 0) {
        // Recheck disabled, so nothing more to do
        stateHandler = 0;
        return;
    }

    if (Time.isValid() && (data->lastCheck + checkPeriod.count()) < Time.now()) {
        // Time to check name again
        // Go to the stateSubscribe because if we have a saved name we might not
        // have added a subscription yet. If we have one we won't subscribe again.
        stateHandler = &DeviceNameHelper::stateSubscribe;
        return;
    }
}



void DeviceNameHelper::subscriptionHandler(const char *eventName, const char *eventData) {

    if (strlen(eventData) < DEVICENAMEHELPER_MAX_NAME_LEN) {
        // Fits
        strcpy(data->name, eventData);
    }
    else {
        // Need to truncate
        strncpy(data->name, eventData, DEVICENAMEHELPER_MAX_NAME_LEN);
        data->name[DEVICENAMEHELPER_MAX_NAME_LEN] = 0;
    }
    gotResponse = true;
}

//
// DeviceNameHelperNoStorage
//

DeviceNameHelperNoStorage &DeviceNameHelperNoStorage::instance() {
    if (!_instance) {
        _instance = new DeviceNameHelperNoStorage();
    }
    return *(DeviceNameHelperNoStorage *)_instance;
}

void DeviceNameHelperNoStorage::setup() {
    this->data = &_data;

    commonSetup();
}

DeviceNameHelperNoStorage::DeviceNameHelperNoStorage() {

}
DeviceNameHelperNoStorage::~DeviceNameHelperNoStorage() {

}


//
// DeviceNameHelperEEPROM
//

DeviceNameHelperEEPROM &DeviceNameHelperEEPROM::instance() {
    if (!_instance) {
        _instance = new DeviceNameHelperEEPROM();
    }
    return *(DeviceNameHelperEEPROM *)_instance;
}

void DeviceNameHelperEEPROM::setup(int eepromStart) {
    this->eepromStart = eepromStart;
    this->data = &eepromData;

    EEPROM.get(eepromStart, eepromData);

    commonSetup();
}

DeviceNameHelperEEPROM::DeviceNameHelperEEPROM() {

}
DeviceNameHelperEEPROM::~DeviceNameHelperEEPROM() {

}

void DeviceNameHelperEEPROM::save() {
    EEPROM.put(eepromStart, eepromData);
}


//
// DeviceNameHelperRetained
//
// [static]
DeviceNameHelperRetained &DeviceNameHelperRetained::instance() {
    if (!_instance) {
        _instance = new DeviceNameHelperRetained();
    }
    return *(DeviceNameHelperRetained *)_instance;
}

void DeviceNameHelperRetained::setup(DeviceNameHelperData *retainedData) {
    this->data = retainedData;

    commonSetup();
}

DeviceNameHelperRetained::DeviceNameHelperRetained() {

}

DeviceNameHelperRetained::~DeviceNameHelperRetained() {

}

#if HAL_PLATFORM_FILESYSTEM

#include <fcntl.h>
#include <sys/stat.h>

//
// DeviceNameHelperFile
//
// [static]
DeviceNameHelperFile &DeviceNameHelperFile::instance() {
    if (!_instance) {
        _instance = new DeviceNameHelperFile();
    }
    return *(DeviceNameHelperFile *)_instance;
}

void DeviceNameHelperFile::setup(const char *path) {
    this->path = path;
    this->data = &fileData;

    // Read file
    int fd = open(path, O_RDWR | O_CREAT);
    if (fd != -1) {
        int count = read(fd, &fileData, sizeof(DeviceNameHelperData));
        if (count != sizeof(DeviceNameHelperData)) {
            // File contents do not appear to be valid; do not use
            memset(&fileData, 0, sizeof(DeviceNameHelperData));
        }
        close(fd);   
    }

    commonSetup();
}

DeviceNameHelperFile::DeviceNameHelperFile() {
}

DeviceNameHelperFile::~DeviceNameHelperFile() {
}

void DeviceNameHelperFile::save() {
    // Save to file
    int fd = open(path, O_RDWR | O_CREAT);
    if (fd != -1) {
        write(fd, &fileData, sizeof(DeviceNameHelperData));
        close(fd);   
    }
}

#endif /* HAL_PLATFORM_FILESYSTEM */