#pragma once

#include <supla/channels/channel.h>
#include <BleScanner.hpp>

class BLE_Sensor {
public:
    enum Type { TermoAndHum, Thermo, Humidity, Open, COUNT };

    inline static const char* TYPE_CODES[Type::COUNT] = {
        [Type::TermoAndHum] = "BLET_TH",
        [Type::Thermo] = "BLET_T",
        [Type::Humidity] = "BLET_H",
        [Type::Open] = "BLET_OPEN",
    };

    inline static const char* TYPE_LABEL[Type::COUNT] = {
        [Type::TermoAndHum] = "Temperatura & Wilgotność",
        [Type::Thermo] = "Temperatura",
        [Type::Humidity] = "Wilgotność",
        [Type::Open] = "Czujnik Otwarcia",
    };


    BLE_Sensor(const String &mac, BleScanner* scanner, uint32_t validTimeMs, Supla::Channel* suplaChannel):
        validTimeMs(validTimeMs),
        lastReceiveTime(0),
        lastValid(true),
        suplaChannel(suplaChannel)
    {
        scanner->addSensor(mac, [this, suplaChannel](String MAC, JsonObject data) {
            // serializeJson(data, Serial);

            lastReceiveTime = millis();
            this->suplaChannel->setStateOnline();

            if (data["batt"].is<double>())
            {
                suplaChannel->setBatteryPowered(true);
                suplaChannel->setBatteryLevel(data["batt"].as<uint8_t>());
                // printf("MAC: %s BAT: %u%%\n", MAC.c_str(), data["batt"].as<uint8_t>());
            }

            if (data["rssi"].is<int>())
            {
                int32_t rssi = map(data["rssi"].as<int>(), BLE_RSSI_MIN, BLE_RSSI_MAX, 0, 100);
                if (rssi >= 0 && rssi <= 100)
                    suplaChannel->setBridgeSignalStrength(rssi);
                else
                    suplaChannel->setBridgeSignalStrength(255);

                // printf("MAC: %s RSSI: %u%%\n", MAC.c_str(), rssi);
            }

            onData(MAC, data);
        });
    }

protected:
    bool isValid() { return millis() - lastReceiveTime < validTimeMs; }

    void init() { suplaChannel->setStateOnlineAndNotAvailable(); }

    void iterate()
    {
        bool valid = isValid();

        if (valid != lastValid)
        {
            lastValid = valid;

            if (!valid)
            {
                onInvalidTime();
                suplaChannel->setStateOnlineAndNotAvailable();
            }
        }
    }

    virtual void onInvalidTime() = 0;

    virtual void onData(const String &MAC, JsonObject data) = 0;

private:
    const uint32_t validTimeMs;
    uint32_t lastReceiveTime;
    bool lastValid;
    Supla::Channel* suplaChannel;

};
