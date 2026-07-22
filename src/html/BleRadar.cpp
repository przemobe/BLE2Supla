#include <html/BleRadar.hpp>


void BleRadar::addResult(JsonObject data)
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

void BleRadar::send(Supla::WebSender *sender)
{
    if (!sender)
    {
        return;
    }

    sender->send("<div class=\"box\"><h3>Wykryte urządzenia</h3><table><tbody><thead><tr><th>Adres MAC</th><th>RSSI</th><th>Nazwa</th><th>Szczegóły</th></tr></thead>");
    std::lock_guard<std::mutex> lck(_entries_mtx);
    for (const Entry &entry : _entries)
    {
        sender->send("<tr><td>");
        sender->send(entry.id.c_str());
        sender->send("</td><td>");
        sender->send(entry.rssi);
        sender->send("</td><td>");
        sender->send(entry.name.c_str());
        sender->send("</td><td>");
        sender->send(entry.info.c_str());
        sender->send("</td></tr>");
    }
    sender->send("</tbody></table></div>");
}
