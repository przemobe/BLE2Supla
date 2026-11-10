#pragma once

#include <supla/channels/channel.h>
#include <BleScanner.hpp>

class BLE_Sensor {
public:
    BLE_Sensor(uint32_t validTimeMs, Supla::Channel &rSuplaChannel):
        validTimeMs(validTimeMs),
        lastReceiveTime(0),
        lastValid(true),
        suplaChannel(rSuplaChannel)
    {}

    virtual ~BLE_Sensor()
    {}

    void procData(const String &MAC, JsonObject data)
    {
        lastReceiveTime = millis();
        suplaChannel.setStateOnline();

        if (data["batt"].is<double>())
        {
            suplaChannel.setBatteryPowered(true);
            suplaChannel.setBatteryLevel(data["batt"].as<uint8_t>());
            // printf("MAC: %s BAT: %u%%\n", MAC.c_str(), data["batt"].as<uint8_t>());
        }

        if (data["rssi"].is<int>())
        {
            int32_t rssi = map(data["rssi"].as<int>(), BLE_RSSI_MIN, BLE_RSSI_MAX, 0, 100);
            if (rssi >= 0 && rssi <= 100)
                suplaChannel.setBridgeSignalStrength(rssi);
            else
                suplaChannel.setBridgeSignalStrength(255);

            // printf("MAC: %s RSSI: %u%%\n", MAC.c_str(), rssi);
        }

        onData(MAC, data);
    }

    void setSubDeviceId(uint8_t subDeviceId)
    {
        suplaChannel.setSubDeviceId(subDeviceId);
    }

    uint8_t getSubDeviceId() const
    {
        return suplaChannel.getSubDeviceId();
    }

protected:
    bool isValid() const { return millis() - lastReceiveTime < validTimeMs; }

    void init() { suplaChannel.setStateOfflineRemoteWakeupNotSupported(); }

    void iterate()
    {
        bool valid = isValid();

        if (valid != lastValid)
        {
            lastValid = valid;

            if (!valid)
            {
                onInvalidTime();
                suplaChannel.setStateOfflineRemoteWakeupNotSupported();
            }
        }
    }

    virtual void onInvalidTime() = 0;

    virtual void onData(const String &MAC, JsonObject data) = 0;

private:
    const uint32_t validTimeMs;
    uint32_t lastReceiveTime;
    bool lastValid;
    Supla::Channel &suplaChannel;

};
