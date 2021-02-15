#ifndef __DEVICENAMEHELPERRK_H
#define __DEVICENAMEHELPERRK_H

// Github: https://github.com/rickkas7/DeviceNameHelperRK
// License: MIT

#include "Particle.h"

/**
 * @brief The maximum name of the device name in characters
 * 
 * The buffer is one byte longer than this; it will always be null terminated.
 * If the device name is longer, it will be truncated. See note in 
 * DeviceNameHelperData about changing this value.
 */
const size_t DEVICENAMEHELPER_MAX_NAME_LEN = 31;

/**
 * @brief Data typically stored in retained memory or EEPROM to avoid having
 * to fetch the name so often.
 * 
 * This structure is currently 44 bytes. It cannot be larger than 255 bytes because
 * the length is stored in a uint8_t. If the structure size is changed, any previously
 * saved data will be discarded and the name fetched again.
 * 
 * Also note that DEVICENAMEHELPER_MAX_NAME_LEN affects the size of this structure.
 */
struct DeviceNameHelperData { // 44 bytes
    /**
     * @brief Magic bytes, DeviceNameHelper::DATA_MAGIC
     * 
     * This value (0x7787a2f2) is used to see if the structure has been initialized
     */
    uint32_t    magic;

    /**
     * @brief Size of this structure, currently 44 bytes. Used to detect when it changes
     * to invalidate the old version
     */
    uint8_t     size;

    /**
     * @brief Flag bits, not currently used. Currently 0.
     */
    uint8_t     flags;

    /**
     * @brief Reserved for future use, currently 0.
     */
    uint16_t    reserved;

    /**
     * @brief Last time the name was checked from Time.now() (seconds past January 1, 1970, UTC).
     */
    long        lastCheck;

    /**
     * @brief The device name
     */
    char        name[DEVICENAMEHELPER_MAX_NAME_LEN + 1];
};

/**
 * @brief Generic base class used by all storage methods
 * 
 * You can't instantiate one of these, you need to instantiate a specific subclass such as 
 * DeviceNameHelperEEPROM, DeviceNameHelperRetained, or DeviceNameHelperNoStorage.
 */
class DeviceNameHelper {
public:
    /**
     * @brief Magic bytes used to detect if EEPROM or retained memory has been initialized
     */
    static const uint32_t DATA_MAGIC = 0x7787a2f2;
    
    /**
     * @brief You must call this from loop on every call to loop()
     */
    void loop();

    /**
     * @brief Adds a function to call when the name is known
     * 
     * @param nameCallback The function to call. It can be a C++11 lambda.
     * 
     * @return *this, so you can chain the withXXX() calls, fluent-style.
     * 
     * The name callback function has the prototype:
     * 
     * void callback(const char *name)
     * 
     * The name is the device name, as a c-string (null terminated).
     */
    DeviceNameHelper &withNameCallback(std::function<void(const char *)> nameCallback);

    /**
     * @brief Sets 
     * 
     * @param checkPeriod How often to check. You can use chrono literals such as 24h for
     * to check once a day, for example.
     * 
     * @return *this, so you can chain the withXXX() calls, fluent-style.
     * 
     * The default is to check once. After the name has been retrieved it will not be retrieved again.
     * This also means that if the name is ever changed, the change would not be detected.
     */
    DeviceNameHelper &withCheckPeriod(std::chrono::seconds checkPeriod) { this->checkPeriod = checkPeriod; return *this; };

    /**
     * @brief Returns true if the name has been retrived and is non-empty
     */
    bool hasName() const { return data && data->name[0] != 0; };

    /**
     * @brief Returns the device name as a c-string
     * 
     * May return an empty string if the name has not been retrieved yet
     */
    const char *getName() const { return data ? data->name : ""; };

    /**
     * @brief Get the time the name was last fetched
     * 
     * Value is from from Time.now(), seconds past January 1, 1970, UTC.
     */
    long getLastNameCheckTime() const { return data ? data->lastCheck : 0; };

    /**
     * @brief Request the name again 
     * 
     * This overrides the periodic check period and requests the name to be checked now,
     * even if it's known and it's not time to check.
     */
    void checkName();

    /**
     * @brief Call if you've called Particle.unsubscribe.
     * 
     * There is no function to remove a single subscription handler; Particle.unsubscribe
     * unsubscribes all handlers. Also, there is no way to tell if a subscription
     * handler has been added when using a C++ class as the handler. Thus we use a flag
     * to keep track if whether we've subscribed or not.
     * 
     * But if you call Particle.unsubscribe() the flag could be set but the subscription
     * would no longer exist, so the subscription wouldn't work and the device name
     * could not be retrieved.
     * 
     * This function is used to rectify this rare condition.
     */
    void subscriptionRemoved() { hasSubscribed = false; };

    /**
     * @brief Special generic instance getter
     * 
     * @return A DeviceNameHelper*, or NULL if it hasn't been instantiated yet.
     * 
     * Normally you use a specific instance getter like DeviceNameHelperEEPROM::instance()
     * which will create the singleton instance if it has not been instantiated yet.
     * 
     * In some rare cases, you may want to get the generic instance pointer, if an 
     * instance has been created. You'd typically do this if you wanted to get the
     * name if DeviceNameHelperNoStorage has been set up, without having to know which
     * storage method was used.
     * 
     * Note that this method will not instantiate the singleton if it does not
     * exist since it doesn't know which one you want. Thus it returns a pointer
     * to the object not a reference, so it can return NULL if the instance
     * does not exist yet.
     */
    static DeviceNameHelper *getInstance() { return _instance; };

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, get a singleton instance of a subclass by using DeviceNameHelperRetained::instance(),
     * DeviceNameHelperEEPROM.instance(), DeviceNameHelperNoStorage::instance(), or DeviceNameHelperFile::instance().
     */
    DeviceNameHelper();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~DeviceNameHelper();

    /**
     * @brief This class is not copyable
     */
    DeviceNameHelper(const DeviceNameHelper&) = delete;

    /**
     * @brief This class is not copyable
     */
    DeviceNameHelper& operator=(const DeviceNameHelper&) = delete;

    /**
     * @brief All of the storage-method specific setup methods call that at the end
     */
    void commonSetup();

    /**
     * @brief This method is called to save the DeviceNameHelperData
     * 
     * This is called always. This base class does nothing. The DeviceNameHelperEEPROM
     * and DeviceNameHelperFile subclasses override this to save the data.
     */
    virtual void save();

    /**
     * @brief State handler, entry point when starting up.
     * 
     * If the device name is saved, this will return quickly without enabling the device 
     * name subscription handler.
     * 
     * Next state:
     * stateWaitRecheck - If the device name is set
     * stateSubscribe - If the device name needs to be retrieved
     */
    void stateStart();

    /**
     * @brief Add a subscription handler, if necessary.
     * 
     * Next state:
     * stateWaitConnected
     */
    void stateSubscribe();

    /**
     * @brief Waits until Particle.connected() and Time.isValid() are true
     * 
     * Next state:
     * stateWaitRequest
     */
    void stateWaitConnected();

    /**
     * @brief Waits POST_CONNECT_WAIT_MS milliseconds (2 seconds) then
     * publishes the request for device name event "particle/device/name"
     * 
     * Next state:
     * stateWaitResponse
     */
    void stateWaitRequest();

    /**
     * @brief Waits for a device name event to be received
     * 
     * If the name is received, it is saved (if necessary) and the name callback
     * is called (if present).
     * 
     * Next state:
     * stateWaitRecheck - name was found
     * stateWaitRetry - timeout (RESPONSE_WAIT_MS, 15 seconds) or empty name
     */
    void stateWaitResponse();

    /**
     * @brief Waits 5 minutes (RETRY_WAIT_MS) and tries requesting the name again
     * 
     * Next state:
     * stateWaitConnected
     */
    void stateWaitRetry();

    /**
     * @brief Wait until it's time to check the name again
     * 
     * Next state:
     * stateSubscribe if it's time to check the name again
     * NULL if we're done
     */
    void stateWaitRecheck();

    /**
     * @brief Subscription handler for the "particle/device/name" event
     * 
     * Since there's no way to unsubscribe a single subscription handler, it's 
     * never removed. See subscriptionRemoved() if you call Particle.unsubscribe()
     * from your code (which is rare).
     */
    void subscriptionHandler(const char *eventName, const char *eventData);

    /**
     * @brief Amount of time to wait after connection for the subscription to be activated (milliseconds)
     */
    static const unsigned long POST_CONNECT_WAIT_MS = 2000;

    /**
     * @brief How long to wait for a device name response before timing out and waiting to retry (milliseconds)
     */
    static const unsigned long RESPONSE_WAIT_MS = 15000;

    /**
     * @brief How long to wait to retry a request to get the device name (in milliseconds)
     */
    static const unsigned long RETRY_WAIT_MS = 5 * 60 * 1000; // 5 minutes

protected:
    /**
     * @brief DeviceNameHelperData structure pointer
     * 
     * For DeviceNameHelperRetained, this points to the actual retained memory. 
     * For all other storage members, it points to a class member variable for the singleton instance.
     */
    DeviceNameHelperData *data = 0;

    /**
     * @brief How often to fetch the name again in seconds (0 = never check again)
     */
    std::chrono::seconds checkPeriod = 0s;

    /**
     * @brief Optional function or C++11 lambda to call when the name is known
     * 
     * This can occur during setup() if the name is saved, otherwise it will occur later, after
     * connecting to the cloud.
     */
    std::function<void(const char *)> nameCallback = 0;

    /**
     * @brief Current state handler, or NULL if in done state
     */
    std::function<void(DeviceNameHelper&)> stateHandler = 0;

    /**
     * @brief Some states use this for timing. It's a value from millis() if used.
     */
    unsigned long stateTime = 0;

    /**
     * @brief true if Particle.subscribe has been called
     */
    bool hasSubscribed = false;

    /**
     * @brief true if the event subscription handler was called. The name is stored in data.name.
     */
    bool gotResponse = false;
    
    /**
     * @brief Used by checkName() to force the name to be checked again
     */
    bool forceCheck = false;

    /**
     * @brief Singleton instance pointer, set by the subclass instance() methods.
     */
    static DeviceNameHelper *_instance;
};

/**
 * @brief Version of DeviceNameHelper that stores the name in volatile RAM
 * 
 * Useful on Gen 2 devices if you don't have sufficient retained RAM or EEPROM available. 
 * The name will be fetched on every startup. On Gen 3 devices you can use 
 * DeviceNameHelperFile with Device OS 2.0.0 and later to store the device name
 * in a file on the 2MB flash file system.
 * 
 * This is not recommended if you are using HIBERNATE sleep mode as the name would
 * need to be fetched on every wake!
 */
class DeviceNameHelperNoStorage : public DeviceNameHelper {
public:
    /**
     * @brief Get the singleton instance of this class, creating it if necessary.
     * 
     * You cannot construct an instance of this class manually, as a global or on
     * the stack. You must instead use instance().
     */
    static DeviceNameHelperNoStorage &instance();

    /**
     * @brief You must call setup() from global setup()!
     * 
     * This is typically done like this from your app's setup() method.
     * 
     * DeviceNameHelperNoStorage::instance().setup();
     * 
     * Also note that you must do the same from global loop():
     * 
     * DeviceNameHelperNoStorage::instance().loop();
     */
    void setup();

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, use DeviceNameHelperNoStorage::instance() to get the singleton instance,
     * creating it if necessary.
     */ 
    DeviceNameHelperNoStorage();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~DeviceNameHelperNoStorage();

    /**
     * @brief Heap-allocated data. A pointer to this is stored in the base class' data field.
     */
    DeviceNameHelperData _data;
};

/**
 * @brief Version of DeviceNameHelper that stores the name in EEPROM emulation
 * 
 * It requires 44 bytes of EEPROM emulation. You specify the start address as 
 * a parameter to setup and the data is a DeviceNameHelperData object.
 * 
 * You must make sure the whole range of values does not interfere with any
 * other data stored in EEPROM. You do not need to initialize the data in
 * any way.
 * 
 * On Gen 3 devices running Device OS 2.0.0 you may want to use a file
 * on the flash file system using DeviceNameHelperFile instead of using
 * EEPROM.
 */
class DeviceNameHelperEEPROM : public DeviceNameHelper {
public:
    /**
     * @brief Get the singleton instance of this class, creating it if necessary.
     * 
     * You cannot construct an instance of this class manually, as a global or on
     * the stack. You must instead use instance().
     */
    static DeviceNameHelperEEPROM &instance();

    /**
     * @brief You must call setup() from global setup()!
     * 
     * This is typically done like this from your app's setup() method.
     * 
     * DeviceNameHelperEEPROM::instance().setup(0);
     * 
     * Also note that you must do the same from global loop():
     * 
     * DeviceNameHelperEEPROM::instance().loop();
     */
    void setup(int eepromStart);

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, use DeviceNameHelperEEPROM::instance() to get the singleton instance,
     * creating it if necessary.
     */ 
    DeviceNameHelperEEPROM();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~DeviceNameHelperEEPROM();

    /**
     * @brief Virtual override of base class to save data to the EEPROM
     */
    virtual void save();

    /**
     * @brief Start offset into the EEPROM for where to save the data
     */
    int eepromStart;

    /**
     * @brief Heap-allocated data. A pointer to this is stored in the base class' data field.
     * 
     * It's read from the EEPROM during setup().
     */
    DeviceNameHelperData eepromData;
};

/**
 * @brief Version of DeviceNameHelper that stores the name in retained RAM.
 * 
 * It requires 44 bytes of retained RAM, out of the 3K or so available on most devices.
 * 
 * This is a good option because the name will be preserved across restarts and 
 * sleep modes. It will often be reset on code flash, however, so a using EEPROM
 * or the file system may be a better choice.
 */
class DeviceNameHelperRetained : public DeviceNameHelper {
public:
    /**
     * @brief Get the singleton instance of this class, creating it if necessary.
     * 
     * You cannot construct an instance of this class manually, as a global or on
     * the stack. You must instead use instance().
     */
    static DeviceNameHelperRetained &instance();

    /**
     * @brief You must call setup() from global setup()!
     * 
     * This is typically done like this from your app's setup() method.
     * 
     * DeviceNameHelperRetained::instance().setup();
     * 
     * Also note that you must do the same from global loop():
     * 
     * DeviceNameHelperRetained::instance().loop();
     */
    void setup(DeviceNameHelperData *retainedData);

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, use DeviceNameHelperRetained::instance() to get the singleton instance,
     * creating it if necessary.
     */ 
    DeviceNameHelperRetained();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~DeviceNameHelperRetained();

};

#if HAL_PLATFORM_FILESYSTEM
/**
 * @brief Store the device name in a file on the flash file system
 * 
 * This is a good option on Gen 3 devices (Argon, Boron, B Series, Tracker SoM) running
 * Device OS 2.0.0 or later. The flash file system is 2 MB (4 MB on Tracker) so there's
 * plenty of space. 
 * 
 * On Gen 3 devices, EEPROM emulation is actually just a file on the flash file system
 * to it's not any more efficient than using a file.
 */
class DeviceNameHelperFile : public DeviceNameHelper {
public:
    /**
     * @brief Get the singleton instance of this class, creating it if necessary.
     * 
     * You cannot construct an instance of this class manually, as a global or on
     * the stack. You must instead use instance().
     */
    static DeviceNameHelperFile &instance();

    /**
     * @brief You must call setup() from global setup()!
     * 
     * This is typically done like this from your app's setup() method.
     * 
     * DeviceNameHelperFile::instance().setup();
     * 
     * Also note that you must do the same from global loop():
     * 
     * DeviceNameHelperFile::instance().loop();
     */
    void setup(const char *path = "/usr/devicename");

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, use DeviceNameHelperFile::instance() to get the singleton instance,
     * creating it if necessary.
     */ 
    DeviceNameHelperFile();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~DeviceNameHelperFile();

    /**
     * @brief Virtual override of base class to save data to the file
     */
    virtual void save();

    /**
     * @brief Path to the data file. Default is "/usr/devicename"
     */
    String path;

    /**
     * @brief Heap-allocated data. A pointer to this is stored in the base class' data field.
     * 
     * It's read from the file system during setup().
     */
    DeviceNameHelperData fileData;
};

#endif /* HAL_PLATFORM_FILESYSTEM */


#endif /* __DEVICENAMEHELPERRK_H */
