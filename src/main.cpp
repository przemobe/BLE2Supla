#include <main.hpp>
#include <HTTPUpdateServer.h>


HTTPUpdateServer httpUpdater;


void setup() {
    Serial.begin(115200);
    Serial.println("Scanning...");

    NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    NimBLEDevice::setScanDuplicateCacheSize(200);
    NimBLEDevice::init(DEVICE_NAME);

    scanner.init();

    // scanner.addSensor("A4:C1:38:??:??:??", [](String MAC, JsonObject data) {
    //     serializeJson(data, Serial);
    //     Serial.println("");
    // });


    cfgButton.configureAsConfigButton(&SuplaDevice);

    SuplaDevice.setName(DEVICE_NAME);

    SuplaDevice.setSuplaCACert(suplaCACert);
    SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
    SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);

    initHtml();
    httpUpdater.setup(suplaServer.getServerPtr(), "/update");

    SuplaDevice.begin();
}


void loop() {
    SuplaDevice.iterate();

    static bool LOCAL_WEB_SERVER = false;
    if (!LOCAL_WEB_SERVER) {
        if (Supla::Network::IsReady()) {
            LOCAL_WEB_SERVER = true;
            SuplaDevice.handleAction(0, Supla::START_LOCAL_WEB_SERVER);
        }
    }

    scanner.iterate();

    delay(10); // For watchdog
}


void initHtml() {
    Supla::Storage::Init();

    new Supla::Html::DeviceInfo(&SuplaDevice);
    new Supla::Html::WifiParameters;
    new Supla::Html::ProtocolParameters;
    new Supla::Html::StatusLedParameters;
    new Supla::Html::DivEnd;

    bleCfg = new Supla::Html::DeviceConfigurator(MAX_DEVICES_COUNT);

    auto setScannerParams = []() {
        printf("BLE Scanning params: %u / %u [s]\n", bleCfg->getScanTime(), bleCfg->getScanInterval());
        scanner.setScanTiming(bleCfg->getScanTime() * 1000, bleCfg->getScanInterval() * 1000);
    };
    setScannerParams();
    bleCfg->OnSaveCallback([setScannerParams]() { setScannerParams(); });



    printf("\n-------------------------------- BLE CONFIG DUMP [%u]\n", bleCfg->getSensorsCount());
    for (size_t q = 0; q < bleCfg->getSensorsCount(); q++) {
        printf("MAC: %s\n", bleCfg->getMAC(q).c_str());

        for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
            if (bleCfg->isType((BLE_Sensor::Type)w, q))
                printf("    - %s\n", BLE_Sensor::TYPE_LABEL[w]);

        printf("--------------------------------\n");
    }
    printf("\n");



    for (size_t q = 0; q < bleCfg->getSensorsCount(); q++) {
        String mac = bleCfg->getMAC(q);
        uint32_t validTimeMs = bleCfg->getValidTime() * 1000;

        for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++) {
            BLE_Sensor::Type type = (BLE_Sensor::Type)w;
            if (bleCfg->isType((BLE_Sensor::Type)w, q))
                BLE_Sensor_Factory::CreateSensor(type, mac, &scanner, validTimeMs);
        }
    }
}
