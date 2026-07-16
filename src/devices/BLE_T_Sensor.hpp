#pragma once

#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <supla/sensor/thermometer.h>

#include "BLE_Sensor.hpp"

class BLE_T_Sensor : public Supla::Sensor::Thermometer, public BLE_Sensor {
public:
    BLE_T_Sensor(String mac, BleScanner* scanner, uint32_t validTimeMs)
        : BLE_Sensor(BLE_Sensor::Type::Thermo, mac, scanner, validTimeMs, &channel)
        , temp(TEMPERATURE_NOT_AVAILABLE) { }

private:
    virtual void onData(String MAC, JsonObject data) {
        if (data["tempc"].is<double>())
            temp = data["tempc"].as<double>();
        else
            temp = TEMPERATURE_NOT_AVAILABLE;


        printf("T SENSOR '%s' -> %0.1f°C\n", MAC.c_str(), temp);

        sendNewValue();
    }

    virtual void onInvalidTime() {
        temp = TEMPERATURE_NOT_AVAILABLE;
        sendNewValue();
    }


    void onInit() override { init(); }
    void iterateAlways() override { iterate(); }

    void sendNewValue() { channel.setNewValue(temp); }


    double temp;
};