#pragma once

#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <supla/sensor/therm_hygro_meter.h>

#include "BLE_Sensor.hpp"


class BLE_TH_Sensor : public BLE_Sensor, public Supla::Sensor::ThermHygroMeter {
public:
    BLE_TH_Sensor(const String &mac, BleScanner* scanner, uint32_t validTimeMs)
        : BLE_Sensor(mac, scanner, validTimeMs, &channel)
        , temp(TEMPERATURE_NOT_AVAILABLE)
        , humi(HUMIDITY_NOT_AVAILABLE) { }


private:
    virtual void onData(const String &MAC, JsonObject data) {
        if (data["tempc"].is<double>())
            temp = data["tempc"].as<double>();
        else
            temp = TEMPERATURE_NOT_AVAILABLE;

        if (data["hum"].is<double>())
            humi = data["hum"].as<double>();
        else
            humi = HUMIDITY_NOT_AVAILABLE;

        printf("TH SENSOR '%s' -> %0.1f°C  %0.0f%%\n", MAC.c_str(), temp, humi);

        sendNewValue();
    }

    virtual void onInvalidTime() {
        temp = TEMPERATURE_NOT_AVAILABLE;
        humi = HUMIDITY_NOT_AVAILABLE;
        sendNewValue();
    }


    void onInit() override { init(); }
    void iterateAlways() override { iterate(); }

    void sendNewValue() { channel.setNewValue(temp, humi); }


    double temp;
    double humi;
};
