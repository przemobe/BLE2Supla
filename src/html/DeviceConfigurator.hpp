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
        #define TAG_COUNT_SELECTOR  "BLE_COUNT"
        #define TAG_INTERVAL_TIME   "BLES_IVAL"
        #define TAG_SCAN_TIME       "BLES_TIME"
        #define TAG_VALID_TIME      "BLES_VALID"

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

        void send(Supla::WebSender* sender)
        {
            auto cfg = Supla::Storage::ConfigInstance();
            if (!cfg)
                return;

            uint8_t count = 1;
            cfg->getUInt8(TAG_COUNT_SELECTOR, &count);

            uint32_t scanInterval = 55;
            cfg->getUInt32(TAG_INTERVAL_TIME, &scanInterval);

            uint32_t scanTime = 5;
            cfg->getUInt32(TAG_SCAN_TIME, &scanTime);

            uint32_t validTime = 240;
            cfg->getUInt32(TAG_VALID_TIME, &validTime);

            sender->send(
            // END BOX PREVIOUS
                "</div>"
                "<div class=\"box\">"
                "<h3>BLE Settings</h3>"

                "<script>"
                    "function formatMac(targ) {"
                        "let value = targ.value.replace(/[^a-fA-F0-9]/g, '');"
                        "value = value.toUpperCase();"
                        "let formattedValue = value.match(/.{1,2}/g)?.join(':') || '';"
                        "targ.value = formattedValue;"
                    "}"
                "</script>"

                "<script>"
                    "function changeCount(val) {"
                        "const maxInputs = 100;"
                        "const visibleCount = parseInt(val);"
                        "for (let i = 1; i <= maxInputs; i++) {"
                            "const input = document.getElementById(`BLEDEV${i}`);"
                            "if (input) {"
                                "input.style.display = (i <= visibleCount) ? '' : 'none';"
                            "}"
                            "else {"
                                "break;"
                            "}"
                        "}"
                    "}"
                "</script>"
                "<script>"
                    "window.onload = function () { "
                    "changeCount(");
            sender->send(count);
            sender->send(
                    ");"
                    "}"
                "</script>"

            // form-field BEGIN
                "<div class=\"form-field\">"
                    "<label for=\"" TAG_INTERVAL_TIME "\">Interwał skanowania [s]</label>"
                    "<input type=\"number\" step=\"1\" name=\"" TAG_INTERVAL_TIME "\" id=\"" TAG_INTERVAL_TIME "\" min=\"10\" max=\"600\" value=\"");
            sender->send(scanInterval);
            sender->send("\">"
                "</div>" // form field

            // form-field BEGIN
                "<div class=\"form-field\">"
                    "<label for=\"" TAG_SCAN_TIME "\">Czas skanowania [s]</label>"
                    "<input type=\"number\" step=\"1\" name=\"" TAG_SCAN_TIME "\" id=\"" TAG_SCAN_TIME "\" min=\"2\" max=\"20\" value=\"");
            sender->send(scanTime);
            sender->send("\">"
                "</div>" // form field

            // form-field BEGIN
                "<div class=\"form-field\">"
                    "<label for=\"" TAG_VALID_TIME "\">Ważność pomiarów [s]</label>"
                    "<input type=\"number\" step=\"1\" name=\"" TAG_VALID_TIME "\" id=\"" TAG_VALID_TIME "\" min=\"20\" max=\"1200\" value=\"");
            sender->send(validTime);
            sender->send("\">"
                "</div>" // form field

            // form-field BEGIN
                "<div class=\"form-field\">"
                    "<label for=\"" TAG_COUNT_SELECTOR "\">Liczba urządzeń BLE</label>"
                    "<select name=\"" TAG_COUNT_SELECTOR "\" id=\"" TAG_COUNT_SELECTOR "\" onchange=\"changeCount(this.value);\">");

            char toSend[512];
            for (size_t q = 0; q < maxCount; q++)
            {
                snprintf(toSend, sizeof(toSend), "<option value=\"%u\"%s>%u</option>", q + 1, ((q + 1) == count) ? " selected" : "", q + 1);
                sender->send(toSend);
            }
            sender->send(
                    "</select>"
                "</div>"); // form field


            char macTag[32];
            char mac[32];

            static const char bledevHtmlTemplate[] = "</div>" // prev BOX
                "<div class=\"box\" id=\"BLEDEV%u\">" // BOX
                    "<h3>Czujnik BLE %u</h3>"
                    "<div class=\"form-field\">"
                        "<label for=\"%s\">Adres MAC</label>"
                        "<input name=\"%s\" id=\"%s\""
                            " maxlength=\"17\""
                            " placeholder=\"AB:CD:EF:12:34:56\""
                            " style=\"text-transform:uppercase;\""
                            " oninput=\"formatMac(this);\""
                            " value=\"%s\">"
                "</div>"; // FORM FIELD

            for (size_t q = 0; q < maxCount; q++)
            {
                snprintf(macTag, 32, "MAC%u", q);
                cfg->getString(macTag, mac, 32);

                snprintf(toSend, sizeof(toSend), bledevHtmlTemplate,
                    q + 1, q + 1, macTag, macTag, macTag, mac);
                sender->send(toSend);

                for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
                {
                    BLE_Sensor_Factory::SendFunctionCheckbox(sender, q + 1, (BLE_Sensor::Type)w);
                }
            }
            sender->send("</div>"); // BOX
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
