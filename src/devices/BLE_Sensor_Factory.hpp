#pragma once

#include <supla/network/web_sender.h>
#include <supla/storage/config.h>

#include <devices/BLE_H_Sensor.hpp>
#include <devices/BLE_Sensor.hpp>
#include <devices/BLE_TH_Sensor.hpp>
#include <devices/BLE_T_Sensor.hpp>
#include <devices/BLE_Open_Sensor.hpp>

class BLE_Sensor_Factory {
public:
    static BLE_Sensor* CreateSensor(BLE_Sensor::Type type, String mac, BleScanner* scanner, uint32_t validTimeMs) {

        switch (type) {
        case BLE_Sensor::Type::TermoAndHum:
            return new BLE_TH_Sensor(mac, scanner, validTimeMs);

        case BLE_Sensor::Type::Thermo:
            return new BLE_T_Sensor(mac, scanner, validTimeMs);

        case BLE_Sensor::Type::Humidity:
            return new BLE_H_Sensor(mac, scanner, validTimeMs);

        case BLE_Sensor::Type::Open:
            return new BLE_Open_Sensor(mac, scanner, validTimeMs);
        }

        return nullptr;
    }

    static void SendFunctionCheckbox(Supla::WebSender* sender, uint8_t devNum, BLE_Sensor::Type type) {
        auto cfg = Supla::Storage::ConfigInstance();

        char tag[32];
        snprintf(tag, 32, "%s%u", BLE_Sensor::TYPE_CODES[type], devNum);

        uint8_t checkedVal = 0;
        cfg->getUInt8(tag, &checkedVal);

        static const char htmlTemplate[] =
            "<div class=\"form-field right-checkbox\">"
                "<label for=\"%s\">%s</label>"
                "<label>"
                    "<span class=\"switch\">"
                        "<input type=\"hidden\" name=\"%s\" value=\"%s\">"
                        "<input type=\"checkbox\" value=\"on\" id=\"%s\"%s onchange=\"document.getElementsByName('%s')[0].value=this.checked?'on':'off'\">"
                        "<span class=\"slider\"></span>"
                    "</span>"
                "</label>"
            "</div>";

        char toSend[512];
        int status = snprintf(toSend, sizeof(toSend), htmlTemplate,
            tag, BLE_Sensor::TYPE_LABEL[(uint8_t)type],
            tag, checkedVal ? "on" : "off",
            tag, checkedVal ? " checked" : "", tag);

        if ((0 > status) || (sizeof(toSend) <= status))
        {
            ESP_LOGE("HTML", "SendFunctionCheckbox fail to create");
            return;
        }
        sender->send(toSend);
    }

    static bool HandleResponse(uint8_t devNum, BLE_Sensor::Type type, const char* key, const char* value) {
        auto cfg = Supla::Storage::ConfigInstance();

        char tag[32];
        snprintf(tag, 32, "%s%u", BLE_Sensor::TYPE_CODES[type], devNum);

        if (strcmp(key, tag) == 0) {
            if (strcmp(value, "on") == 0)
                cfg->setUInt8(tag, 1);
            else if (strcmp(value, "off") == 0)
                cfg->setUInt8(tag, 0);
            return true;
        }
        return false;
    }

    static bool IsType(uint8_t devNum, BLE_Sensor::Type type) {
        auto cfg = Supla::Storage::ConfigInstance();

        char tag[32];
        snprintf(tag, 32, "%s%u", BLE_Sensor::TYPE_CODES[type], devNum);

        uint8_t checkedVal = 0;
        cfg->getUInt8(tag, &checkedVal);
        return checkedVal;
    }
};
