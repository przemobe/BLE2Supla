#include <string.h>
#include <supla/network/web_sender.h>
#include <supla/storage/config.h>
#include <supla/storage/storage.h>
#include <supla/tools.h>
#include <supla/device/remote_device_config.h>
#include <supla/device/status_led.h>
#include <supla/storage/config_tags.h>

#include "config.hpp"
#include "html/BoardConfigHtml.hpp"


static const char statusLedGpioTag[] = "BLE_LED_STAT_GPIO";
static const char statusLedModeTag[] = "BLE_LED_STAT_MODE";

uint8_t BoardConfigHtml::getStatusLedGpio()
{
    uint8_t statusLedGpioValue = GPIO_LED_NUM;
    auto cfg = Supla::Storage::ConfigInstance();
    if (cfg)
    {
        cfg->getUInt8(statusLedGpioTag, &statusLedGpioValue);
    }
    return statusLedGpioValue;
}

BoardConfigHtml::LedGpioMode BoardConfigHtml::getStatusLedGpioMode()
{
    uint8_t statusLedGpioModeValue = (GPIO_LED_INV ? LedGpioMode_Inverted : LedGpioMode_NonInverted);
    auto cfg = Supla::Storage::ConfigInstance();
    if (cfg)
    {
        cfg->getUInt8(statusLedModeTag, &statusLedGpioModeValue);
    }
    return static_cast<LedGpioMode>(statusLedGpioModeValue);
}

void BoardConfigHtml::send(Supla::WebSender* sender)
{
    const uint8_t statusLedGpioValue = getStatusLedGpio();
    const LedGpioMode statusLedGpioModeValue = getStatusLedGpioMode();

    /*
    sender->send(
        "<div class=\"box\">"
            "<h3>Board Configuration</h3>");
    */

    sender->formField([&]()
    {
        sender->labelFor(statusLedGpioTag, "Status LED GPIO number");
        sender->numberInput(statusLedGpioTag,
        {
            .min = 0,
            .max = 64,
            .value = statusLedGpioValue,
            .step = 1,
        });
    });

    sender->labeledField(statusLedModeTag, "Status LED GPIO mode", [&]()
    {
        sender->selectTag(statusLedModeTag, statusLedModeTag).body([&]()
        {
            sender->selectOption(LedGpioMode_NonInverted, "Non-Inverted", statusLedGpioModeValue == LedGpioMode_NonInverted);
            sender->selectOption(LedGpioMode_Inverted, "Inverted", statusLedGpioModeValue == LedGpioMode_Inverted);
        });
    });

    /*
    sender->send(
        "</div>");
    */
}

bool BoardConfigHtml::handleResponse(const char* key, const char* value)
{
    auto cfg = Supla::Storage::ConfigInstance();
    if (!cfg)
    {
        return false;
    }

    if (strcmp(key, statusLedGpioTag) == 0)
    {
        uint8_t param = static_cast<uint8_t>(stringToUInt(value));
        cfg->setUInt8(statusLedGpioTag, param);
        return true;
    }

    if (strcmp(key, statusLedModeTag) == 0)
    {
        uint8_t param = static_cast<uint8_t>(stringToUInt(value));
        if (LedGpioMode_COUNT <= param)
        {
            param = LedGpioMode_COUNT - 1;
        }
        cfg->setUInt8(statusLedModeTag, param);
        return true;
    }

    return false;
}

void BoardConfigHtml::makeSuplaStatusLed()
{
    const uint8_t statusLedGpioValue = getStatusLedGpio();
    const LedGpioMode statusLedGpioModeValue = getStatusLedGpioMode();

    switch (statusLedGpioModeValue)
    {
        case LedGpioMode_NonInverted:
        case LedGpioMode_Inverted:
            new Supla::Device::StatusLed(statusLedGpioValue, (LedGpioMode_Inverted == statusLedGpioModeValue));
            break;

        default:
            break;
    }
}
