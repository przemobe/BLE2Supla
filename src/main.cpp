#include "main.hpp"
#include <HTTPUpdateServer.h>
#include "html/ScannerResults.hpp"


HTTPUpdateServer gHTTPUpdateServer;
ScannerResults gScannerResults;


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
    gHTTPUpdateServer.setup(suplaServer.getServerPtr(), "/update");

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
}


void initHtml() {
    Supla::Storage::Init();

    new Supla::Html::DeviceInfo(&SuplaDevice);
    new Supla::Html::WifiParameters;
    new Supla::Html::ProtocolParameters;
    new Supla::Html::StatusLedParameters;

    bleCfg = new Supla::Html::DeviceConfigurator(MAX_DEVICES_COUNT);
    new ScanResultsHtml(gScannerResults);

    const uint8_t deviceCount = bleCfg->getDeviceCount();

    auto setScannerParams = []()
    {
        printf("BLE Scanning params: %u / %u [s]\n", bleCfg->getScanTime(), bleCfg->getScanInterval());
        scanner.setScanTiming(bleCfg->getScanTime() * 1000, bleCfg->getScanInterval() * 1000);
    };

    auto updateDeviceKeys = [deviceCount]()
    {
        uint8_t bleKey[16];
        for (uint8_t devIdx = 0; devIdx < deviceCount; devIdx++)
        {
            bool keyStatus = bleCfg->getBKey(devIdx, bleKey);
            if (keyStatus)
            {
                const uint64_t u64mac = static_cast<uint64_t>(NimBLEAddress(bleCfg->getMAC(devIdx).c_str(), 0));
                scanner.setBleKey(devIdx, u64mac, bleKey);
                printf("Key set for devIdx=%u MAC=%012llX\n", devIdx, u64mac);
            }
            else
            {
                scanner.clearBleKey(devIdx);
            }
        }
    };
    setScannerParams();
    updateDeviceKeys();

    bleCfg->OnSaveCallback([setScannerParams, updateDeviceKeys]()
    {
        setScannerParams();
        updateDeviceKeys();
    });

    printf("\n-------------------------------- BLE CONFIG DUMP [%u]\n", deviceCount);

    for (uint8_t devIdx = 0; devIdx < deviceCount; devIdx++)
    {
        printf("MAC: %s\n", bleCfg->getMAC(devIdx).c_str());

        for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
        {
            if (bleCfg->isType((BLE_Sensor::Type)w, devIdx))
            {
                printf("    - %s\n", BLE_Sensor::TYPE_LABEL[w]);
            }
        }

        printf("--------------------------------\n");
    }
    printf("\n");


    const uint32_t validTimeMs = bleCfg->getValidTime() * 1000;
    for (uint8_t devIdx = 0; devIdx < deviceCount; devIdx++)
    {
        String mac = bleCfg->getMAC(devIdx);

        for (uint8_t w = 0; w < (uint8_t)BLE_Sensor::Type::COUNT; w++)
        {
            BLE_Sensor::Type type = (BLE_Sensor::Type)w;
            if (bleCfg->isType(type, devIdx))
            {
                BLE_Sensor_Factory::CreateSensor(type, mac, &scanner, validTimeMs);
            }
        }
    }
}
