#include <BleScanner.hpp>


const NimBLEUUID BleScanner::NIM_BLEUUID_BTHOME(BTHOME_UUID);


BleScanner::BleScanner():
    pBLEScan(nullptr),
    pBtHomeHandle(nullptr),
    lastScanMillis(0),
    scanTimeMillis(5 * 1000),
    scanIntervalMillis(10 * 1000)
{
    for (size_t q = 0; q < MAX_SENSORS; q++) {
        sensorsID[q].cb = nullptr;
        sensorsID[q].ID.clear();
    }
}



void BleScanner::init() {
    pBLEScan = NimBLEDevice::getScan(); // create new scan
    pBLEScan->setScanCallbacks(this, false);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(97);
    pBLEScan->setWindow(37);
    pBLEScan->setMaxResults(0);
}


void BleScanner::iterate() {
    bool firstScan = true;

    if ((millis() - lastScanMillis > scanIntervalMillis || firstScan) && pBLEScan && !pBLEScan->isScanning()) {
        lastScanMillis = millis();
        firstScan = false;

        pBLEScan->start(scanTimeMillis);

#ifdef APP_DEBUG
        printf("BLE Scanner: Starting Scan!\n");
#endif
    }
}


void BleScanner::setScanTiming(unsigned long scanTimeMillis, unsigned long scanIntervalMillis) {
    this->scanTimeMillis = scanTimeMillis;
    this->scanIntervalMillis = scanIntervalMillis;
}


void BleScanner::addSensor(String ID, CallbackFun_t cb) {
    ID.toUpperCase();

    if (!MAC::isValid(ID)) {
        ESP_LOGE(TAG, "Can't register new mac, bad format! = \'%s\'", ID.c_str());
        return;
    }

    for (size_t q = 0; q < MAX_SENSORS; q++) {
        if (sensorsID[q].cb == nullptr) {
            sensorsID[q].cb = cb;
            sensorsID[q].ID = ID.c_str();
            return;
        }
    }
    ESP_LOGE(TAG, "Can't register new mac, list is full!");
}


String BleScanner::hexifyString(const std::string &deviceServiceData) {
    String hexString = "";
    for (unsigned int i = 0; i < deviceServiceData.length(); i++) {
        byte b = deviceServiceData[i];
        if (b < 0x10)
            hexString += "0";

        hexString += String(b, HEX);
    }
    return hexString;
}


void BleScanner::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {

    JsonObject BLEdata = doc.to<JsonObject>();

    const std::vector<uint8_t> &payload = advertisedDevice->getPayload();
#ifdef APP_DEBUG
    printf("[BLE] payload(%u)=%s\n", payload.size(), hexifyString({(const char*)payload.data(), payload.size()}).c_str());
#endif

    String mac_adress = advertisedDevice->getAddress().toString().c_str();
    mac_adress.toUpperCase();

    BLEdata["id"] = (char*)mac_adress.c_str();
    BLEdata["rssi"] = (int)advertisedDevice->getRSSI();

    if (advertisedDevice->haveName())
    {
        BLEdata["name"] = (char*)advertisedDevice->getName().c_str();
    }

    if (advertisedDevice->haveTXPower())
    {
        BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();
    }

    if (advertisedDevice->haveManufacturerData())
    {
        BLEdata["manufacturerdata"] = hexifyString(advertisedDevice->getManufacturerData()).c_str();
    }

    const uint8_t serviceDataCount = advertisedDevice->getServiceDataCount();
    for (uint8_t j = 0; j < serviceDataCount; j++)
    {
        BLEdata["servicedata"] = hexifyString(advertisedDevice->getServiceData(j)).c_str();
        BLEdata["servicedatauuid"] = advertisedDevice->getServiceDataUUID(j).toString().c_str();
    }

    if (serviceDataCount && (advertisedDevice->getServiceDataUUID(0) == NIM_BLEUUID_BTHOME) && decodeBtHome(BLEdata, payload))
    {
#ifdef APP_DEBUG
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
#endif

        for (size_t q = 0; q < MAX_SENSORS; q++) {
            if (sensorsID[q].ID.length() == 0)
                continue;
            if (MAC::compare(mac_adress, sensorsID[q].ID)) {
                if (sensorsID[q].cb != nullptr) {
                    sensorsID[q].cb(sensorsID[q].ID, BLEdata);
                }
            }
        }
    }
    else if (decoder.decodeBLEJson(BLEdata))
    {
#ifdef APP_DEBUG
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
#endif

        BLEdata.remove("manufacturerdata");
        BLEdata.remove("servicedata");
        BLEdata.remove("servicedatauuid");
        BLEdata.remove("type");
        BLEdata.remove("cidc");
        BLEdata.remove("acts");
        BLEdata.remove("cont");
        BLEdata.remove("track");

        for (size_t q = 0; q < MAX_SENSORS; q++) {
            if (sensorsID[q].ID.length() == 0)
                continue;
            if (MAC::compare(mac_adress, sensorsID[q].ID)) {
                if (sensorsID[q].cb != nullptr) {
                    sensorsID[q].cb(sensorsID[q].ID, BLEdata);
                }
            }
        }
    }
#ifdef APP_DEBUG
    else
    {
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
    }
#endif
}


bool BleScanner::decodeBtHome(JsonObject &BLEdata, const std::vector<uint8_t> &payload)
{
    if (nullptr == pBtHomeHandle)
    {
        esp_err_t ret = bthome_create(&pBtHomeHandle);
        if (ESP_OK != ret)
        {
            ESP_LOGE(TAG, "[BTHOME] bthome_create ERROR=%u\n", ret);
            return false;
        }
    }

    bthome_reports_t *ptReports = bthome_parse_adv_data(pBtHomeHandle, payload.data(), payload.size());
    const uint8_t num_reports = (ptReports ? std::min(ptReports->num_reports, (uint8_t)BTHOME_REPORTS_MAX) : 0);

    for (uint8_t rptIdx = 0; rptIdx < num_reports; rptIdx++)
    {
        bthome_report_t &report = ptReports->report[rptIdx];
        printf("[BTHOME] rptIdx=%u/%u id=0x%02x len=%u data=%s\n", rptIdx+1, num_reports, report.id, report.len,
            hexifyString({(const char*)report.data, report.len}).c_str());

        switch (report.id)
        {
            case BTHOME_SENSOR_ID_TEMPERATURE_0X01:
            {
                printf("[BTHOME] val=%f\n", *((int16_t*)report.data) * 0.01d);
                BLEdata["tempc"] = *((int16_t*)report.data) * 0.01d;
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X10:
            {
                printf("[BTHOME] val=%f\n", *((int16_t*)report.data) * 0.1d);
                BLEdata["tempc"] = *((int16_t*)report.data) * 0.1d;
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_1X00:
            {
                printf("[BTHOME] val=%f\n", *((int8_t*)report.data) * 1.0d);
                BLEdata["tempc"] = *((int8_t*)report.data) * 1.0d;
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X35:
            {
                printf("[BTHOME] val=%f\n", *((int8_t*)report.data) * 0.35d);
                BLEdata["tempc"] = *((int8_t*)report.data) * 0.35d;
                break;
            }

            default:
                break;
        }
    }

    bthome_free_reports(ptReports);

    return true;
}
