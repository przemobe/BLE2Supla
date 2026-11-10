#pragma once

#include <supla/element.h>
#include <supla/protocol/supla_srpc.h>

class SubDeviceInfoReporter : public Supla::Element
{
protected:
    uint8_t nextDevIdx = 0;
    uint8_t deviceCount = 0;
    unsigned long prevSendTime = 0;
    Supla::Protocol::SuplaSrpc *srpc = nullptr;

public:
    void onRegistered(Supla::Protocol::SuplaSrpc *srpc) override;
    bool iterateConnected() override;
};
