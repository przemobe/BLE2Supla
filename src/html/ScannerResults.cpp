#include "html/ScannerResults.hpp"
#include <cstring>


void ScannerResults::addResult(JsonObject data)
{
    if (not data["id"].is<const char*>())
    {
        return;
    }

    Entry newEntry = {};

    strncpy(newEntry.id, data["id"].as<const char*>(), sizeof(newEntry.id) - 1);

    if (data["name"].is<const char*>())
    {
        strncpy(newEntry.name, data["name"].as<const char*>(), sizeof(newEntry.name) - 1);
    }

    if (data["rssi"].is<int>())
    {
        newEntry.rssi = data["rssi"].as<int>();
    }

    char *infoPtr = newEntry.info;
    if (data["tempc"].is<double>())
    {
        *infoPtr++ = 'T';
    }

    if (data["_tempc"].is<double>())
    {
        *infoPtr++ = 'T';
    }

    if (data["hum"].is<double>())
    {
        *infoPtr++ = 'H';
    }

    if (data["_hum"].is<double>())
    {
        *infoPtr++ = 'H';
    }

    if (data["open"].is<bool>())
    {
        *infoPtr++ = 'O';
    }

    if (data["batt"].is<double>())
    {
        *infoPtr++ = 'B';
    }

    std::lock_guard<std::mutex> lck(_entries_mtx);
    const auto it = std::find_if(_entries.begin(), _entries.end(), [&newEntry](const Entry &entry)
        {
            return (0 == strncmp(entry.id, newEntry.id, sizeof(entry.id)));
        });
    if (_entries.end() != it)
    {
        _entries.erase(it);
    }
    else if (_entries.size() > 32)
    {
        _entries.pop_back();
    }

    _entries.push_front(newEntry);
}

void snprintfSafeHtmlStr(char *out, size_t outSize, const char *in)
{
    if (0 == outSize)
    {
        return;
    }

    outSize--;
    for (; *in && outSize; in++)
    {
        switch (*in)
        {
            case '\'':
                strncpy(out, "&apos;", outSize);
                outSize += 6;
                out += 6;
                break;

            case '"':
                strncpy(out, "&quot;", outSize);
                outSize += 6;
                out += 6;
                break;

            case '<':
                strncpy(out, "&lt;", outSize);
                outSize += 4;
                out += 4;
                break;

            case '>':
                strncpy(out, "&gt;", outSize);
                outSize += 4;
                out += 4;
                break;

            case '&':
                strncpy(out, "&amp;", outSize);
                outSize += 5;
                out += 5;
                break;

            default:
                *out = *in;
                out++;
                outSize--;
        }
    }

    *out = '\0';
}

void ScannerResults::send(Supla::WebSender *sender)
{
    if (!sender)
    {
        return;
    }

    char safeName[128];
    char toSend[256];

    sender->send(
        "<div class=\"box\">"
            "<h3>Wykryte urzadzenia</h3>"
            "<table><thead><tr><th>Adres MAC</th><th>RSSI</th><th>Nazwa</th><th>Czujniki</th></tr></thead>"
                "<tbody>");

    {
        std::lock_guard<std::mutex> lck(_entries_mtx);
        for (const Entry &entry : _entries)
        {
            snprintfSafeHtmlStr(safeName, sizeof(safeName), entry.name);
            snprintf(toSend, sizeof(toSend),
                        "<tr><td>%s</td><td>%d</td><td>%s</td><td>%s</td></tr>",
                        entry.id, entry.rssi, safeName, entry.info);
            sender->send(toSend);
        }
    }

    sender->send(
                "</tbody>"
            "</table>"
        // "</div>" // end tag is sent by Supla
    );
}

ScanResultsHtml::ScanResultsHtml(ScannerResults &rBleRadarResults):
    _rBleRadarResults(rBleRadarResults)
{
}

void ScanResultsHtml::send(Supla::WebSender *sender)
{
    _rBleRadarResults.send(sender);
}
