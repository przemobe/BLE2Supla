#include <supla/network/web_sender.h>
#include <supla/storage/config.h>
#include <supla/storage/storage.h>
#include <supla/tools.h>

#include "config.hpp"
#include "html/BoardConfigHtml.hpp"


static const char statusLedGpioTag[] = "BLE_LED_STAT_GPIO";
static const char statusLedModeTag[] = "BLE_LED_STAT_MODE";

BoardLedIo BoardConfigHtml::suplaStatusLedIo = {};
Supla::Device::StatusLed *BoardConfigHtml::pStatusLed = nullptr;


void BoardLedIo::customPinMode(int channelNumber, uint8_t pin, uint8_t mode)
{
    pinMode(pin, mode);
}

void BoardLedIo::customDigitalWrite(int channelNumber, uint8_t pin, uint8_t val)
{
    (void) channelNumber;

    if (LedGpioMode_NonInverted == ledGpioMode)
    {
        digitalWrite(pin, val);
        return;
    }

    if (LedGpioMode_Inverted == ledGpioMode)
    {
        digitalWrite(pin, !val);
        return;
    }

    // WS2812B / RGB modes
    if (!val)
    {
        rgbLedWrite(pin, 0, 0, 0);
        return;
    }

    switch (ledGpioMode)
    {
        case LedGpioMode_RGB_White100:
            rgbLedWrite(pin, 255, 255, 255);
            break;

        case LedGpioMode_RGB_White50:
            rgbLedWrite(pin, 127, 127, 127);
            break;

        case LedGpioMode_RGB_White25:
            rgbLedWrite(pin, 63, 63, 63);
            break;

        case LedGpioMode_RGB_Red:
            rgbLedWrite(pin, 255, 0, 0);
            break;

        case LedGpioMode_RGB_Green:
            rgbLedWrite(pin, 0, 255, 0);
            break;

        case LedGpioMode_RGB_Blue:
            rgbLedWrite(pin, 0, 0, 255);
            break;

        default:
            rgbLedWrite(pin, 255, 255, 255);
            break;
    }

    return;
}

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

LedGpioMode BoardConfigHtml::getStatusLedGpioMode()
{
    uint8_t statusLedGpioModeValue = std::min(static_cast<uint8_t>(GPIO_LED_MODE), static_cast<uint8_t>(LedGpioMode_COUNT - 1));
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
            sender->selectOption(LedGpioMode_RGB_White100, "WS2812 White 100%", statusLedGpioModeValue == LedGpioMode_RGB_White100);
            sender->selectOption(LedGpioMode_RGB_White50, "WS2812 White 50%", statusLedGpioModeValue == LedGpioMode_RGB_White50);
            sender->selectOption(LedGpioMode_RGB_White25, "WS2812 White 25%", statusLedGpioModeValue == LedGpioMode_RGB_White25);
            sender->selectOption(LedGpioMode_RGB_Red, "WS2812 Red", statusLedGpioModeValue == LedGpioMode_RGB_Red);
            sender->selectOption(LedGpioMode_RGB_Green, "WS2812 Green", statusLedGpioModeValue == LedGpioMode_RGB_Green);
            sender->selectOption(LedGpioMode_RGB_Blue, "WS2812 Blue", statusLedGpioModeValue == LedGpioMode_RGB_Blue);
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
        // GPIO change needs restart to apply.
        uint8_t newVal = static_cast<uint8_t>(stringToUInt(value));
        cfg->setUInt8(statusLedGpioTag, newVal);
        return true;
    }

    if (strcmp(key, statusLedModeTag) == 0)
    {
        uint8_t newVal = static_cast<uint8_t>(stringToUInt(value));
        if (LedGpioMode_COUNT <= newVal)
        {
            newVal = LedGpioMode_COUNT - 1;
        }

        uint8_t oldVal = GPIO_LED_NUM;
        cfg->getUInt8(statusLedModeTag, &oldVal);
        if (newVal != oldVal)
        {
            cfg->setUInt8(statusLedModeTag, newVal);
            if (pStatusLed)
            {
                suplaStatusLedIo.setLedGpioMode(static_cast<LedGpioMode>(newVal));
                pStatusLed->identify();
            }
        }

        return true;
    }

    return false;
}

void BoardConfigHtml::makeSuplaStatusLed()
{
    const uint8_t statusLedGpioValue = getStatusLedGpio();
    const LedGpioMode statusLedGpioModeValue = getStatusLedGpioMode();

    suplaStatusLedIo.setLedGpioMode(statusLedGpioModeValue);
    pStatusLed = new Supla::Device::StatusLed(&suplaStatusLedIo, statusLedGpioValue, false);
}
