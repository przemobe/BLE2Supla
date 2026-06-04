#include <BleScanner.hpp>


BleScanner::BleScanner() {
    for (size_t q = 0; q < MAX_SENSORS; q++) {
        sensorsID[q].cb = nullptr;
        sensorsID[q].ID.clear();
    }

    lastScanMillis = 0;

    scanTimeMillis = 5 * 1000;
    scanIntervalMillis = 10 * 1000;
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

    if ((millis() - lastScanMillis > scanIntervalMillis || firstScan) && !pBLEScan->isScanning()) {
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


String BleScanner::hexifyString(std::string deviceServiceData) {
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

    String mac_adress = advertisedDevice->getAddress().toString().c_str();
    mac_adress.toUpperCase();


    BLEdata["id"] = (char*)mac_adress.c_str();
    BLEdata["rssi"] = (int)advertisedDevice->getRSSI();

    if (advertisedDevice->haveName())
        BLEdata["name"] = (char*)advertisedDevice->getName().c_str();

    if (advertisedDevice->haveTXPower())
        BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();

    if (advertisedDevice->haveManufacturerData())
        BLEdata["manufacturerdata"] = hexifyString(advertisedDevice->getManufacturerData()).c_str();

    if (advertisedDevice->haveServiceData()) {
        int serviceDataCount = advertisedDevice->getServiceDataCount();
        for (int j = 0; j < advertisedDevice->getServiceDataCount(); j++) {
            BLEdata["servicedata"] = hexifyString(advertisedDevice->getServiceData(j)).c_str();
            BLEdata["servicedatauuid"] = advertisedDevice->getServiceDataUUID(j).toString().c_str();
        }
    }

    if (decoder.decodeBLEJson(BLEdata)) {

#ifdef APP_DEBUG
        serializeJson(BLEdata, Serial);
        Serial.println("\n-------------------------------------------------------------------------------------------");
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
}