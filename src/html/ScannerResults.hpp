#pragma once

#include <string>
#include <deque>
#include <mutex>
#include <supla/network/web_sender.h>
#include <supla/network/html_element.h>

//#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

class ScannerResults
{
public:
    void addResult(JsonObject data);
    void send(Supla::WebSender *sender);

protected:
    struct Entry
    {
        int rssi;
        char id[18];
        char name[18];
        char info[8];
    };

    typedef std::deque<Entry> Entries;

    Entries _entries;
    std::mutex _entries_mtx;
};


class ScanResultsHtml : public Supla::HtmlElement
{
public:
    ScanResultsHtml(ScannerResults &rBleRadarResults);
    void send(Supla::WebSender *sender);

protected:
    ScannerResults &_rBleRadarResults;
};
