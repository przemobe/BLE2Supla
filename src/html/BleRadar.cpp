#include <html/BleRadar.hpp>


void BleRadarResults::addResult(JsonObject data)
{
    if (not data["id"].is<const char*>())
    {
        return;
    }

    Entry newEntry = {};

    newEntry.id = data["id"].as<const char*>();

    if (data["name"].is<const char*>())
    {
        newEntry.name = data["name"].as<const char*>();
    }

    if (data["rssi"].is<int>())
    {
        newEntry.rssi = data["rssi"].as<int>();
    }

    std::lock_guard<std::mutex> lck(_entries_mtx);
    const auto it = std::find_if(_entries.begin(), _entries.end(), [&newEntry](const Entry &entry){ return entry.id == newEntry.id; });
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

    sender->send("<div class=\"box\"><h3>Wykryte urzadzenia</h3><table><thead><tr><th>Adres MAC</th><th>RSSI</th><th>Nazwa</th><th>Szczegoly</th></tr></thead><tbody>");
    std::lock_guard<std::mutex> lck(_entries_mtx);
    for (const Entry &entry : _entries)
    {
        sender->tag("tr").body([&]()
        {
            sender->tag("td").body(entry.id.c_str());
            sender->tag("td").body([&]() { sender->send(entry.rssi); });
            sender->tag("td").body([&]() { sender->sendSafe(entry.name.c_str()); });
            sender->tag("td").body([&]() { sender->sendSafe(entry.info.c_str()); });
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
