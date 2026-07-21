#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <functional>

#include <supla/network/html_element.h>
#include <supla/network/html_generator.h>
#include <supla/network/web_sender.h>
#include <supla/storage/config.h>
#include <supla/storage/storage.h>

#include <devices/BLE_Sensor_Factory.hpp>

namespace Supla {
namespace Html {

    class DeviceConfigurator : public Supla::HtmlElement {
    public:
        typedef std::function<void()> OnSaveCb_t;

    private:
        constexpr static const char* TAG_COUNT_SELECTOR = "BLE_COUNT";

        constexpr static const char* TAG_INTERVAL_TIME = "BLES_IVAL";
        constexpr static const char* TAG_SCAN_TIME = "BLES_TIME";
        constexpr static const char* TAG_VALID_TIME = "BLES_VALID";


        size_t maxCount = 16;

        OnSaveCb_t cb = nullptr;




    public:
        DeviceConfigurator(size_t maxCount)
            : HtmlElement(HTML_SECTION_FORM)
            , maxCount(maxCount) { }

        void OnSaveCallback(OnSaveCb_t cb) { this->cb = cb; }

        uint32_t getScanInterval() {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return 0;

            uint32_t scanInterval = 55;
            cfg->getUInt32(TAG_INTERVAL_TIME, &scanInterval);
            return scanInterval;
        }

        uint32_t getScanTime() {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return 0;

            uint32_t scanTime = 5;
            cfg->getUInt32(TAG_SCAN_TIME, &scanTime);
            return scanTime;
        }

        uint32_t getValidTime() {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return 0;

            uint32_t validTime = 240;
            cfg->getUInt32(TAG_VALID_TIME, &validTime);
            return validTime;
        }

        void send(Supla::WebSender* sender) {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return;

            sender->send("<div class=\"box\">");
            sender->send("<h3>BLE Settings</h3>");

            sender->send("<script>"
                         "function formatMac(targ) {"
                         "let value = targ.value.replace(/[^a-fA-F0-9]/g, '');"
                         "value = value.toUpperCase();"
                         "let formattedValue = value.match(/.{1,2}/g)?.join(':') || '';"
                         "targ.value = formattedValue;"
                         "}</script>");

            sender->send("<script>"
                         "function changeCount(val) {"
                         "const maxInputs = 100;"
                         "const visibleCount = parseInt(val);"
                         "for (let i = 1; i <= maxInputs; i++) {"
                         "const input = document.getElementById(`BLEDEV${i}`);"
                         "if (input)"
                         "{input.style.display = (i <= visibleCount) ? '' : 'none';}"
                         "else"
                         "{break;}"
                         "}"
                         "}</script>");


            uint8_t count = 1;
            cfg->getUInt8(TAG_COUNT_SELECTOR, &count);

            sender->send("<script>"
                         "window.onload = function () { "
                         "changeCount(");
            sender->send(count);
            sender->send(");"
                         "}"
                         "</script>");


            uint32_t scanInterval = 55;
            cfg->getUInt32(TAG_INTERVAL_TIME, &scanInterval);
            // form-field BEGIN
            sender->send("<div class=\"form-field\">");
            sender->sendLabelFor(TAG_INTERVAL_TIME, "Interwał Skanowania [s]");
            sender->send("<input type=\"number\" step=\"1\" ");
            sender->sendNameAndId(TAG_INTERVAL_TIME);
            sender->send(" min=\"10\" max=\"600\" value=\"");
            sender->send(scanInterval);
            sender->send("\">");
            sender->send("</div>");

            uint32_t scanTime = 5;
            cfg->getUInt32(TAG_SCAN_TIME, &scanTime);
            // form-field BEGIN
            sender->send("<div class=\"form-field\">");
            sender->sendLabelFor(TAG_SCAN_TIME, "Czas skanowania [s]");
            sender->send("<input type=\"number\" step=\"1\" ");
            sender->sendNameAndId(TAG_SCAN_TIME);
            sender->send(" min=\"2\" max=\"20\" value=\"");
            sender->send(scanTime);
            sender->send("\">");
            sender->send("</div>");

            uint32_t validTime = 240;
            cfg->getUInt32(TAG_VALID_TIME, &validTime);
            // form-field BEGIN
            sender->send("<div class=\"form-field\">");
            sender->sendLabelFor(TAG_VALID_TIME, "Ważność pomiarów [s]");
            sender->send("<input type=\"number\" step=\"1\" ");
            sender->sendNameAndId(TAG_VALID_TIME);
            sender->send(" min=\"20\" max=\"1200\" value=\"");
            sender->send(validTime);
            sender->send("\">");
            sender->send("</div>");

            // form-field BEGIN
            sender->send("<div class=\"form-field\">");
            sender->sendLabelFor(TAG_COUNT_SELECTOR, "Ilość urządzeń BLE");

            sender->send("<select");
            sender->sendNameAndId(TAG_COUNT_SELECTOR);
            sender->send("onchange=\"changeCount(this.value);\">");

            for (size_t q = 0; q < maxCount; q++) {
                sender->send("<option value=\"");
                sender->send(q + 1);
                sender->send("\"");

                sender->send(selected((q + 1) == count));

                sender->send(">");
                sender->send(q + 1);
                sender->send("</option>");
            }
            sender->send("</select></div>"); // form field

            sender->send("</div>");



            for (size_t q = 0; q < maxCount; q++) {
                char n[32];
                snprintf(n, 32, "MAC%u", q);

                char mac[32] = "";
                cfg->getString(n, mac, 32);

                sender->send("<div class=\"box\" id=\"BLEDEV"); // BOX
                sender->send(q + 1);
                sender->send("\">");

                sender->send("<h3>Czujnik BLE ");
                sender->send(q + 1);
                sender->send("</h3>");

                sender->send("<div class=\"form-field\">");
                sender->sendLabelFor(n, "Adres MAC");
                sender->send("<input ");
                sender->sendNameAndId(n);
                sender->send("maxlength=\"17\"");
                sender->send("placeholder=\"AB:CD:EF:12:34:56\"");
                sender->send("style=\"text-transform:uppercase;\"");
                sender->send("oninput=\"formatMac(this);\"");
                sender->send("value=\"");
                sender->send(mac);
                sender->send("\">");
                sender->send("</div>"); // FORM FIELD

                for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
                    BLE_Sensor_Factory::SendFunctionCheckbox(sender, q + 1, (BLE_Sensor::Type)w);

                sender->send("</div>"); // BOX
            }
        } // sender


        bool handleResponse(const char* key, const char* value) {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return false;


            if (strcmp(key, TAG_INTERVAL_TIME) == 0) {
                uint32_t param = (uint32_t)stringToInt(value);
                cfg->setUInt32(TAG_INTERVAL_TIME, param);
                return true;
            }
            if (strcmp(key, TAG_SCAN_TIME) == 0) {
                uint32_t param = (uint32_t)stringToInt(value);
                cfg->setUInt32(TAG_SCAN_TIME, param);
                return true;
            }
            if (strcmp(key, TAG_VALID_TIME) == 0) {
                uint32_t param = (uint32_t)stringToInt(value);
                cfg->setUInt32(TAG_VALID_TIME, param);
                return true;
            }


            if (strcmp(key, TAG_COUNT_SELECTOR) == 0) {
                uint8_t param = (uint8_t)stringToInt(value);
                cfg->setUInt8(TAG_COUNT_SELECTOR, param);
                return true;
            }


            for (size_t q = 0; q < maxCount; q++) {
                char n[32];
                snprintf(n, 32, "MAC%u", q);
                if (strcmp(n, key) == 0) {
                    cfg->setString(n, value);
                    return true;
                }

                for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
                    if (BLE_Sensor_Factory::HandleResponse(q + 1, (BLE_Sensor::Type)w, key, value))
                        return true;
            }

            return false;
        }

        void onProcessingEnd() {
            auto cfg = Supla::Storage::ConfigInstance();
            cfg->saveIfNeeded();

            if (cb != nullptr)
                cb();
        }

        uint8_t getSensorsCount() {
            auto cfg = Supla::Storage::ConfigInstance();
            uint8_t count = 0;
            cfg->getUInt8(TAG_COUNT_SELECTOR, &count);
            return count;
        }

        String getMAC(size_t idx) {
            if (idx > maxCount)
                return "";
            auto cfg = Supla::Storage::ConfigInstance();

            char tag[32];
            snprintf(tag, 32, "MAC%u", idx);

            char val[32];

            cfg->getString(tag, val, 32);

            return String(val);
        }

        bool isType(BLE_Sensor::Type type, uint8_t devNum) { return BLE_Sensor_Factory::IsType(devNum + 1, type); }

    }; // DeviceConfigurator

}; // namespace Html
}; // namespace Supla
