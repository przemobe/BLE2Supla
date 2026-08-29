#pragma once

#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <supla/sensor/therm_hygro_meter.h>

#include "BLE_Sensor.hpp"


class BLE_TH_Sensor : public Supla::Sensor::ThermHygroMeter, public BLE_Sensor {
public:
    BLE_TH_Sensor(const String &mac, BleScanner* scanner, uint32_t validTimeMs)
        : BLE_Sensor(mac, scanner, validTimeMs, &channel)
        , temp(TEMPERATURE_NOT_AVAILABLE)
        , humi(HUMIDITY_NOT_AVAILABLE) { }


private:
    virtual void onData(const String &MAC, JsonObject data)
    {
        bool noTemp = false;

        if (data["tempc"].is<double>())
        {
            temp = data["tempc"].as<double>();
            data.remove("tempc");
        }
        else if (data["_tempc"].is<double>())
        {
            temp = data["_tempc"].as<double>();
        }
        else
        {
            noTemp = true;
        }

        if (data["hum"].is<double>())
        {
            humi = data["hum"].as<double>();
            data.remove("hum");
        }
        else if (data["_hum"].is<double>())
        {
            humi = data["_hum"].as<double>();
        }
        else if (noTemp)
        {
            return;
        }

        printf("TH SENSOR '%s' -> %0.1f°C  %0.0f%%\n", MAC.c_str(), temp, humi);

        sendNewValue();
    }

    virtual void onInvalidTime()
    {
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
