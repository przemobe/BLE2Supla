#pragma once
#include <esp_log.h>
#include <supla/log_wrapper.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include <NimBLEDevice.h>
#include <decoder.h>

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

    void setScanTiming(unsigned long scanTimeMillis, unsigned long scanIntervalMillis);

private:
    constexpr static const char* TAG = "BleScanner";
    constexpr static const size_t MAX_SENSORS = MAX_SENSORS_COUNT;

    struct Callback_t {
        CallbackFun_t cb;
        String ID;
    };
    Callback_t sensorsID[MAX_SENSORS];



    static String hexifyString(const std::string &deviceServiceData);



    TheengsDecoder decoder;
    JsonDocument doc;

    NimBLEScan* pBLEScan;
    unsigned long lastScanMillis;

    unsigned long scanTimeMillis;
    unsigned long scanIntervalMillis;
};
