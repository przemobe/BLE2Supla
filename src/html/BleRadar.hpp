#pragma once

#include <string>
#include <deque>
#include <mutex>
#include <supla/network/web_sender.h>
#include <supla/network/html_element.h>

//#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

class BleRadar : public Supla::HtmlElement
{
public:
    void addResult(JsonObject data);
    void send(Supla::WebSender *sender);

protected:
    struct Entry
    {
        std::string id;
        std::string name;
        int rssi;
        std::string info;
    };

    typedef std::deque<Entry> Entries;

    Entries _entries;
    std::mutex _entries_mtx;
};
