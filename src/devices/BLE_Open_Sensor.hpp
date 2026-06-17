#pragma once

#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <supla/sensor/binary.h>

#include "BLE_Sensor.hpp"


class BLE_Open_Sensor : public BLE_Sensor, public Supla::Sensor::BinaryBase {
public:
    BLE_Open_Sensor(const String &mac, BleScanner* scanner, uint32_t validTimeMs)
        : BLE_Sensor(mac, scanner, validTimeMs, &channel)
        , open(false) { }

private:
    virtual void onData(const String &MAC, JsonObject data) {
        if (data["open"].is<bool>()){}
            open = data["open"].as<bool>();

        printf("OPEN SENSOR '%s' -> %s\n", MAC.c_str(), open ? "open" : "close");
        sendNewValue();
    }

    virtual void onInvalidTime() { sendNewValue(); }

    virtual bool getValue() { return open; }


    void onInit() override { init(); }
    void iterateAlways() override { iterate(); }

    void sendNewValue() { channel.setNewValue(open); }

    bool open;
};
