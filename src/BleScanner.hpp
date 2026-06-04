#pragma once
#include <esp_log.h>
#include <supla/log_wrapper.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include <NimBLEDevice.h>
#include <decoder.h>
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
    void callSensors(const String &ID, JsonObject &BLEdata);

    void setScanTiming(unsigned long scanTimeMillis, unsigned long scanIntervalMillis);

private:
    constexpr static const char* TAG = "BleScanner";
    constexpr static const size_t MAX_SENSORS = MAX_SENSORS_COUNT;
    static const NimBLEUUID NIM_BLEUUID_BTHOME;

    struct Callback_t {
        CallbackFun_t cb;
        String ID;
    };
    Callback_t sensorsID[MAX_SENSORS];



    static String hexifyString(const std::string &deviceServiceData);
    bool decodeBtHome(JsonObject &BLEdata, const std::vector<uint8_t> &payload);



    TheengsDecoder decoder;
    JsonDocument doc;

    NimBLEScan* pBLEScan;
    bthome_handle_t pBtHomeHandle;
    unsigned long lastScanMillis;

    unsigned long scanTimeMillis;
    unsigned long scanIntervalMillis;
};
