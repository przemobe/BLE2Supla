#pragma once

#include <string>
#include <unordered_map>
#include <supla/network/web_sender.h>
//#include <supla/network/html_element.h>

//#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

class BleRadar // : public Supla::HtmlElement
{
public:
    void addResult(JsonObject data);
    void send(Supla::WebSender *sender);

protected:
    struct Entry
    {
        std::string name;
        int rssi;
    };

    typedef std::unordered_map<std::string, Entry> Entries; // key: address

    Entries entries;
};
