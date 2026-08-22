#include "BleScanner.hpp"
#include "html/ScannerResults.hpp"


const NimBLEUUID BleScanner::NIM_BLEUUID_BTHOME(BTHOME_UUID);
extern ScannerResults gScannerResults;


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
    static bool firstScan = true;

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


void BleScanner::callSensors(const String &ID, JsonObject BLEdata)
{
    for (size_t q = 0; q < MAX_SENSORS; q++)
    {
        if (sensorsID[q].ID.length() == 0)
            continue;

        if (MAC::compare(ID, sensorsID[q].ID))
        {
            if (sensorsID[q].cb != nullptr)
            {
                sensorsID[q].cb(sensorsID[q].ID, BLEdata);
            }
        }
    }
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

    if (serviceDataCount &&
        (advertisedDevice->getServiceDataUUID(0) == NIM_BLEUUID_BTHOME) &&
        decodeBtHome(BLEdata, *advertisedDevice))
    {
#ifdef APP_DEBUG
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
#endif

        gScannerResults.addResult(BLEdata);
        callSensors(mac_adress, BLEdata);
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

        gScannerResults.addResult(BLEdata);
        callSensors(mac_adress, BLEdata);
    }
    else
    {
#ifdef APP_DEBUG
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
#endif

        gScannerResults.addResult(BLEdata);
    }
}


template <typename ValueT>
void addResultValue(JsonObject BLEdata, std::string key, ValueT value)
{
    while (BLEdata[key].is<ValueT>())
    {
        key.insert(0, 1, '_');
    }

    BLEdata[key] = value;
}


bool BleScanner::decodeBtHome(JsonObject BLEdata, const NimBLEAdvertisedDevice &advertisedDevice)
{
    esp_err_t ret = ESP_OK;
    if (nullptr == pBtHomeHandle)
    {
        ret = bthome_create(&pBtHomeHandle);
        if (ESP_OK != ret)
        {
            ESP_LOGE(TAG, "[BTHOME] bthome_create ERROR=%u\n", ret);
            return false;
        }
    }

    NimBLEAddress peerMacAddr = advertisedDevice.getAddress();
    peerMacAddr.reverseByteOrder();
    ret = bthome_set_peer_mac_addr(pBtHomeHandle, peerMacAddr.getVal());
    if (ESP_OK != ret)
    {
        ESP_LOGE(TAG, "[BTHOME] bthome_set_peer_mac_addr ERROR=%u\n", ret);
        return false;
    }

    uint8_t peerKey[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // TODO: get peer key.
    ret = bthome_set_encrypt_key(pBtHomeHandle, peerKey);
    if (ESP_OK != ret)
    {
        ESP_LOGE(TAG, "[BTHOME] bthome_set_encrypt_key ERROR=%u\n", ret);
        return false;
    }

    const std::vector<uint8_t> &payload = advertisedDevice.getPayload();
    bthome_reports_t *ptReports = bthome_parse_adv_data(pBtHomeHandle, payload.data(), payload.size());
    if (nullptr == ptReports)
    {
        ESP_LOGE(TAG, "[BTHOME] bthome_parse_adv_data fail\n");
        return false;
    }

    const uint8_t num_reports = std::min(ptReports->num_reports, (uint8_t)BTHOME_REPORTS_MAX);
    for (uint8_t rptIdx = 0; rptIdx < num_reports; rptIdx++)
    {
        const bthome_report_t &report = ptReports->report[rptIdx];
#ifdef APP_DEBUG
        printf("[BTHOME] rptIdx=%u/%u id=0x%02x len=%u data=%s\n", rptIdx+1, num_reports, report.id, report.len,
            hexifyString({(const char*)report.data, report.len}).c_str());
#endif

        // BTHome data is LE, assume system is also LE so conversion can be skipped.
        switch (report.id)
        {
            case BTHOME_SENSOR_ID_BATTERY:
            {
                addResultValue(BLEdata, "batt", *((uint8_t*)report.data) * 1.0d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X01:
            {
                addResultValue(BLEdata, "tempc", *((int16_t*)report.data) * 0.01d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X10:
            {
                addResultValue(BLEdata, "tempc", *((int16_t*)report.data) * 0.1d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_1X00:
            {
                addResultValue(BLEdata, "tempc", *((int8_t*)report.data) * 1.0d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X35:
            {
                addResultValue(BLEdata, "tempc", *((int8_t*)report.data) * 0.35d);
                break;
            }

            case BTHOME_SENSOR_ID_HUMIDITY_0X01:
            {
                addResultValue(BLEdata, "hum", *((uint16_t*)report.data) * 0.01d);
                break;
            }

            case BTHOME_SENSOR_ID_HUMIDITY_1x00:
            {
                addResultValue(BLEdata, "hum", *((uint8_t*)report.data) * 1.0d);
                break;
            }

            case BTHOME_BIN_SENSOR_ID_GENERIC:
            case BTHOME_BIN_SENSOR_ID_POWER:
            case BTHOME_BIN_SENSOR_ID_OPENING:
            case BTHOME_BIN_SENSOR_ID_BATTERY ... BTHOME_BIN_SENSOR_ID_WINDOW:
            {
                addResultValue(BLEdata, "open", (bool)(0x01 & *(uint8_t*)report.data));
                break;
            }

            default:
                break;
        }
    }

    bthome_free_reports(ptReports);

    return true;
}
