#include <html/BleRadar.hpp>


void BleRadar::addResult(JsonObject data)
{
    if (not data["id"].is<const char*>())
    {
        return;
    }

    Entry entry = {};
    if (data["name"].is<const char*>())
    {
        entry.name = data["name"].as<const char*>();
    }

    if (data["rssi"].is<int>())
    {
        entry.rssi = data["rssi"].as<int>();
    }

    std::string key = data["id"].as<const char*>();
    entries[key] = entry;

    while (32 < entries.size())
    {
        entries.erase(entries.begin());
    }
}

void BleRadar::send(Supla::WebSender *sender)
{
    if (!sender)
    {
        return;
    }

    sender->send("<div class=\"box\"><h3>Wykryte urządzenia</h3><table><tbody><thead><tr><th>Adres MAC</th><th>RSSI</th><th>Nazwa</th><th>Szczegóły</th></tr></thead>");
    for (auto &kv : entries)
    {
        const Entry &entry = kv.second;
        sender->send("<tr><td>");
        sender->send(kv.first.c_str());
        sender->send("</td><td>");
        sender->send(entry.rssi);
        sender->send("</td><td>");
        sender->send(entry.name.c_str());
        sender->send("</td><td>-</td></tr>");
    }
    sender->send("</tbody></table></div>");
}
