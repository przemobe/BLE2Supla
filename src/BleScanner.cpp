#include "BleScanner.hpp"
#include "html/ScannerResults.hpp"
#include "XMiBeacon.hpp"


const NimBLEUUID BleScanner::NIM_BLEUUID_BTHOME(BTHOME_UUID);
const NimBLEUUID BleScanner::NIM_BLEUUID_XMIBEACON(XMIBEACON_UUID);
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

    for (MacBleKey &rMacBleKey : macBleKeys)
    {
        rMacBleKey.u64mac = 0;
    }
}



void BleScanner::init() {
    pBLEScan = NimBLEDevice::getScan(); // create new scan
    if (pBLEScan)
    {
        pBLEScan->setScanCallbacks(this, false);
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(97);
        pBLEScan->setWindow(37);
        pBLEScan->setMaxResults(0);
    }
    else
    {
        ESP_LOGE(TAG, "NimBLEDevice::getScan() failed!");
    }
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


String BleScanner::hexifyString(const std::string_view &data)
{
    static const char hexChars[] = "0123456789abcdef";
    const size_t dataLength = data.length();

    String hexString;
    hexString.reserve(dataLength * 2 + 1);

    for (size_t i = 0; i < dataLength; i++)
    {
        uint8_t b = data[i];
        hexString += hexChars[b >> 4];
        hexString += hexChars[0x0F & b];
    }

    return hexString;
}


void BleScanner::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {

    JsonObject BLEdata = doc.to<JsonObject>();

    const std::vector<uint8_t> &payload = advertisedDevice->getPayload();
#ifdef APP_DEBUG
    printf("[BLE] payload(%u)=%s\n", payload.size(), hexifyString({(const char*)payload.data(), payload.size()}).c_str());
#endif

    String mac_address = advertisedDevice->getAddress().toString().c_str();
    mac_address.toUpperCase();

    BLEdata["id"] = (char*)mac_address.c_str();
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
        callSensors(mac_address, BLEdata);
    }
    else if (serviceDataCount &&
        (advertisedDevice->getServiceDataUUID(0) == NIM_BLEUUID_XMIBEACON) &&
        decodeXMiBeacon(BLEdata, *advertisedDevice))
    {
#ifdef APP_DEBUG
        std::string serializedJson;
        serializeJson(BLEdata, serializedJson);
        printf("[BLE] Json=%s\n", serializedJson.c_str());
        printf("-------------------------------------------------------------------------------------------\n");
#endif

        gScannerResults.addResult(BLEdata);
        callSensors(mac_address, BLEdata);
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
        callSensors(mac_address, BLEdata);
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

    const std::string serviceData = advertisedDevice.getServiceData(0);
    bthome_device_info_t info;

    info.all = serviceData[0];

    if (info.bit.encryption_flag)
    {
        addResultValue(BLEdata, "encr", true);

        NimBLEAddress peerMacAddr = advertisedDevice.getAddress();
        const uint64_t u64mac = static_cast<uint64_t>(peerMacAddr);

        const uint8_t* peerKey = findBleKey(u64mac);
        if (nullptr == peerKey)
        {
            ESP_LOGW(TAG, "[BTHOME] Cannot acquire key for MAC=%012llX", u64mac);
            return false;
        }

#ifdef APP_DEBUG
        printf("[BTHOME] peerKey=%s\n", BleScanner::hexifyString({(const char*)peerKey, 16}).c_str());
#endif

        ret = bthome_set_encrypt_key(pBtHomeHandle, peerKey);
        if (ESP_OK != ret)
        {
            ESP_LOGE(TAG, "[BTHOME] bthome_set_encrypt_key ERROR=%u\n", ret);
            return false;
        }

        peerMacAddr.reverseByteOrder();
        ret = bthome_set_peer_mac_addr(pBtHomeHandle, peerMacAddr.getVal());
        if (ESP_OK != ret)
        {
            ESP_LOGE(TAG, "[BTHOME] bthome_set_peer_mac_addr ERROR=%u\n", ret);
            return false;
        }
    }

    bthome_reports_t *ptReports = bthome_parse_service_data(pBtHomeHandle, (const uint8_t*)serviceData.c_str(), serviceData.size());
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

        // BTHome data is Little Endian
        switch (report.id)
        {
            case BTHOME_SENSOR_ID_BATTERY:
            {
                addResultValue(BLEdata, "batt", *((uint8_t*)report.data) * 1.0d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X01:
            {
                uint16_t u16val = report.data[0] | (report.data[1] << 8);
                addResultValue(BLEdata, "tempc", static_cast<int16_t>(u16val) * 0.01d);
                break;
            }

            case BTHOME_SENSOR_ID_TEMPERATURE_0X10:
            {
                uint16_t u16val = report.data[0] | (report.data[1] << 8);
                addResultValue(BLEdata, "tempc", static_cast<int16_t>(u16val) * 0.1d);
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
                uint16_t u16val = report.data[0] | (report.data[1] << 8);
                addResultValue(BLEdata, "hum", u16val * 0.01d);
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


bool BleScanner::decodeXMiBeacon(JsonObject BLEdata, const NimBLEAdvertisedDevice &advertisedDevice)
{
    const std::string serviceData = advertisedDevice.getServiceData(0);
    const bool isEncrypted = xMiIsServiceDataEncrypted((uint8_t*)serviceData.c_str());
    const NimBLEAddress peerMacAddr = advertisedDevice.getAddress();
    const uint8_t *macRev = peerMacAddr.getVal();
    const uint8_t *bindKey = nullptr;

#ifdef APP_DEBUG
    printf("[XMI] serviceData len(%u) enc(%u)=%s\n", serviceData.size(), isEncrypted, BleScanner::hexifyString({serviceData.c_str(), serviceData.size()}).c_str());
#endif

    if (isEncrypted)
    {
        addResultValue(BLEdata, "encr", true);

        const uint64_t u64mac = static_cast<uint64_t>(peerMacAddr);
        bindKey = findBleKey(u64mac);
        if (nullptr == bindKey)
        {
            ESP_LOGW(TAG, "[XMI] Cannot acquire BIND KEY for MAC=%012llX", u64mac);
            return false;
        }

#ifdef APP_DEBUG
        printf("[XMI] revMAC=%s\n", BleScanner::hexifyString({(const char*)macRev, 6}).c_str());
        printf("[XMI] bindKey=%s\n", BleScanner::hexifyString({(const char*)bindKey, 16}).c_str());
#endif
    }

    const uint8_t *dataPointer = nullptr;
    size_t dataLength = 0;
    uint8_t buffer[64];
    if (false == xMiBeaconGetData((const uint8_t*)serviceData.c_str(), serviceData.size(), bindKey, macRev, buffer, dataPointer, dataLength))
    {
        ESP_LOGE(TAG, "[XMI] xMiBeaconGetData fail");
        return false;
    }

#ifdef APP_DEBUG
    printf("[XMI] data(%u)=%s\n", dataLength, BleScanner::hexifyString({(const char*)dataPointer, dataLength}).c_str());
#endif

    // Loop through the TLV container data chunk
    size_t index = 0;
    uint32_t reportCnt = 0;
    while (index + 3 <= dataLength)
    {
        const uint16_t objectId = dataPointer[index] | (dataPointer[index + 1] << 8);
        const uint8_t objLength = dataPointer[index + 2];
        index += 3;

        if (index + objLength > dataLength)
        {
            break;
        }

        switch (objectId)
        {
            case 0x1004:
            {
                // Temperature
                int16_t rawTemp = dataPointer[index] | (dataPointer[index + 1] << 8);
#ifdef APP_DEBUG
                printf("[XMI] 0x1004 Temperature: %.1f °C\n", rawTemp / 10.0);
#endif
                addResultValue(BLEdata, "tempc", rawTemp * 0.1d);
                reportCnt++;
                break;
            }
            case 0x1006:
            {
                // Humidity
                uint16_t rawHum = dataPointer[index] | (dataPointer[index + 1] << 8);
#ifdef APP_DEBUG
                printf("[XMI] 0x1006 Humidity: %.1f %%\n", rawHum / 10.0);
#endif
                addResultValue(BLEdata, "hum", rawHum * 0.01d);
                reportCnt++;
                break;
            }
            case 0x100D:
            {
                // Combo Temperature & Humidity
                int16_t rawTemp = dataPointer[index] | (dataPointer[index + 1] << 8);
                uint16_t rawHum = dataPointer[index + 2] | (dataPointer[index + 3] << 8);
#ifdef APP_DEBUG
                printf("[XMI] 0x100D Temperature: %.1f °C\n", rawTemp / 10.0);
                printf("             Humidity: %.1f %%\n", rawHum / 10.0);
#endif
                addResultValue(BLEdata, "tempc", rawTemp * 0.1d);
                addResultValue(BLEdata, "hum", rawHum * 0.1d);
                reportCnt++;
                break;
            }
            case 0x100A:
            {
                // Battery
#ifdef APP_DEBUG
                printf("[XMI] 0x100A Battery: %d %%\n", dataPointer[index]);
#endif
                addResultValue(BLEdata, "batt", dataPointer[index] * 1.0d);
                reportCnt++;
                break;
            }
            case 0x4c01:
            {
                // Temperature
                float fval;
                memcpy(&fval, dataPointer + index, sizeof(fval));
#ifdef APP_DEBUG
                printf("[XMI] 0x4c01 Temperature: %.2f °C\n", fval);
#endif
                addResultValue(BLEdata, "tempc", fval * 1.0d);
                reportCnt++;
                break;
            }
            case 0x4c02:
            {
                // Humidity
#ifdef APP_DEBUG
                printf("[XMI] 0x4c02 Humidity: %u %%\n", dataPointer[index]);
#endif
                addResultValue(BLEdata, "hum", dataPointer[index] * 1.0d);
                reportCnt++;
                break;
            }
            case 0x4c03:
            {
                // Battery
#ifdef APP_DEBUG
                printf("[XMI] 0x4c03 Battery: %u %%\n", dataPointer[index]);
#endif
                addResultValue(BLEdata, "batt", dataPointer[index] * 1.0d);
                reportCnt++;
                break;
            }
            case 0x4c08:
            {
                // Humidity
                float fval;
                memcpy(&fval, dataPointer + index, sizeof(fval));
#ifdef APP_DEBUG
                printf("[XMI] 0x4c08 Humidity: %.2f %%\n", fval);
#endif
                addResultValue(BLEdata, "hum", fval * 1.0d);
                reportCnt++;
                break;
            }
            default:
            {
                ESP_LOGW(TAG, "[XMI] Unknown objectId(0x%x) len(%u)", objectId, objLength);
                break;
            }
        }
        index += objLength;
    }

    if (0 == reportCnt)
    {
        ESP_LOGE(TAG, "[XMI] No objectId is decoded!");
        return false;
    }

    return true;
}


const uint8_t * BleScanner::findBleKey(const uint64_t u64mac) const
{
    for (const MacBleKey &rMacBleKey : macBleKeys)
    {
        if (rMacBleKey.u64mac == u64mac)
        {
            return rMacBleKey.bleKey;
        }
    }
    return nullptr;
}


void BleScanner::setBleKey(const size_t devIdx, const uint64_t u64mac, const uint8_t *bleKey)
{
    if (MAX_DEVICES_COUNT <= devIdx)
    {
        return;
    }
    macBleKeys[devIdx].u64mac = u64mac;
    memcpy(macBleKeys[devIdx].bleKey, bleKey, sizeof(macBleKeys[devIdx].bleKey));
}


void BleScanner::clearBleKey(const size_t devIdx)
{
    if (MAX_DEVICES_COUNT <= devIdx)
    {
        return;
    }
    macBleKeys[devIdx].u64mac = 0;
}
