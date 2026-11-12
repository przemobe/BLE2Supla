#include <Arduino.h>
#include <string.h>
#include "SubDeviceInfoReporter.hpp"
#include "html/DeviceConfigurator.hpp"


extern Supla::Html::DeviceConfigurator* bleCfg;


void SubDeviceInfoReporter::onRegistered(Supla::Protocol::SuplaSrpc *srpc)
{
    this->srpc = srpc;
    restart();
}

bool SubDeviceInfoReporter::iterateConnected()
{
    if (!srpc || !bleCfg)
    {
        return true;
    }

    if (deviceCount <= nextDevIdx)
    {
        return true;
    }

    const unsigned long currTime = millis();
    if (currTime - prevSendTime < 1000)
    {
        return true;
    }

    TDS_SubdeviceDetails subdeviceDetails = {};

    subdeviceDetails.SubDeviceId = nextDevIdx + 1;
    snprintf(subdeviceDetails.Name, SUPLA_DEVICE_NAME_MAXSIZE - 1, "BLE %u", subdeviceDetails.SubDeviceId);
    // char SoftVer[SUPLA_SOFTVER_MAXSIZE]; - unused
    // char ProductCode[SUPLA_SUBDEVICE_PRODUCT_CODE_MAXSIZE]; - unused
    strncpy(subdeviceDetails.SerialNumber, bleCfg->getMAC(nextDevIdx).c_str(), SUPLA_SUBDEVICE_SERIAL_NUMBER_MAXSIZE - 1);

    srpc->sendSubdeviceDetails(&subdeviceDetails);

    nextDevIdx++;
    prevSendTime = currTime;

    return false;
}

void SubDeviceInfoReporter::restart()
{
    prevSendTime = millis();
    nextDevIdx = 0;
    deviceCount = (bleCfg ? bleCfg->getDeviceCount() : 0);
}
