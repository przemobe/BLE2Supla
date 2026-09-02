#pragma once
#include <esp_log.h>
#include <supla/log_wrapper.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include <NimBLEDevice.h>
#include <decoder.h>
#include <string_view>

#include "bthome_v2.h"
#include "MAC.hpp"
#include "config.hpp"


class BleScanner : public NimBLEScanCallbacks {
public:
    typedef std::function<void(String MAC, JsonObject data)> CallbackFun_t;

    BleScanner();

    void init();
    void iterate();

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    void addSensor(String MAC, CallbackFun_t cb);
    void callSensors(const String &ID, JsonObject BLEdata);

    void setScanTiming(unsigned long scanTimeMillis, unsigned long scanIntervalMillis);

    const uint8_t * findBleKey(const uint64_t u64mac) const;
    void setBleKey(const size_t devIdx, const uint64_t u64mac, const uint8_t *bleKey);
    void clearBleKey(const size_t devIdx);

private:
    constexpr static const char* TAG = "BleScanner";
    constexpr static const size_t MAX_SENSORS = MAX_SENSORS_COUNT;
    static const NimBLEUUID NIM_BLEUUID_BTHOME;
    static const NimBLEUUID NIM_BLEUUID_XMIBEACON;

    struct Callback_t {
        CallbackFun_t cb;
        String ID;
    };
    Callback_t sensorsID[MAX_SENSORS];


    static String hexifyString(const std::string_view &data);
    bool decodeBtHome(JsonObject BLEdata, const NimBLEAdvertisedDevice &advertisedDevice);
    bool decodeXMiBeacon(JsonObject BLEdata, const NimBLEAdvertisedDevice &advertisedDevice);



    TheengsDecoder decoder;
    JsonDocument doc;

    NimBLEScan* pBLEScan;
    bthome_handle_t pBtHomeHandle;
    unsigned long lastScanMillis;

    unsigned long scanTimeMillis;
    unsigned long scanIntervalMillis;

    struct MacBleKey
    {
        uint64_t u64mac;
        uint8_t bleKey[16];
    } macBleKeys[MAX_DEVICES_COUNT];
};
