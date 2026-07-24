#include <html/BleRadar.hpp>
#include <cstring>


void BleRadarResults::addResult(JsonObject data)
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

/*
 * https://forum.supla.org/viewtopic.php?t=18368
 * https://supla.github.io/supla-device/classSupla_1_1WebSender.html
 */
void BleRadarResults::send(Supla::WebSender *sender)
{
    if (!sender)
    {
        return;
    }

    sender->send("<div class=\"box\"><h3>Wykryte urzadzenia</h3><table><thead><tr><th>Adres MAC</th><th>RSSI</th><th>Nazwa</th><th>Czujniki</th></tr></thead><tbody>");
    std::lock_guard<std::mutex> lck(_entries_mtx);
    for (const Entry &entry : _entries)
    {
        sender->tag("tr").body([&]()
        {
            sender->tag("td").body(entry.id);
            sender->tag("td").body([&]() { sender->send(entry.rssi); });
            sender->tag("td").body([&]() { sender->sendSafe(entry.name); });
            sender->tag("td").body([&]() { sender->sendSafe(entry.info); });
        });
    }
    sender->send("</tbody></table></div>");
}

BleRadarHtml::BleRadarHtml(BleRadarResults &rBleRadarResults):
    _rBleRadarResults(rBleRadarResults)
{
}

void BleRadarHtml::send(Supla::WebSender *sender)
{
    _rBleRadarResults.send(sender);
}
