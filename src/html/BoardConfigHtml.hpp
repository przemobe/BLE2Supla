#pragma once

#include <stdint.h>
#include <supla/network/web_sender.h>
#include <supla/network/html_element.h>
#include <supla/device/status_led.h>

enum LedGpioMode
{
    LedGpioMode_NonInverted = 0,
    LedGpioMode_Inverted = 1,
    LedGpioMode_RGB_White100 = 2,
    LedGpioMode_RGB_White50 = 3,
    LedGpioMode_RGB_White25 = 4,
    LedGpioMode_RGB_Red = 5,
    LedGpioMode_RGB_Green = 6,
    LedGpioMode_RGB_Blue = 7,
    LedGpioMode_COUNT,
};

class BoardLedIo : public Supla::Io::Base
{
public:
    void customPinMode(int channelNumber, uint8_t pin, uint8_t mode) override;
    void customDigitalWrite(int channelNumber, uint8_t pin, uint8_t val) override;
    int customDigitalRead(int channelNumber, uint8_t pin) override { return 0; }
    void setLedGpioMode(LedGpioMode ledGpioModeIn) { ledGpioMode = ledGpioModeIn; }

protected:
    LedGpioMode ledGpioMode = LedGpioMode_Inverted;
};

class BoardConfigHtml : public Supla::HtmlElement
{
public:
    static uint8_t getStatusLedGpio();
    static LedGpioMode getStatusLedGpioMode();

    void send(Supla::WebSender* sender) override;
    bool handleResponse(const char* key, const char* value) override;

    static void makeSuplaStatusLed();

protected:
    static BoardLedIo suplaStatusLedIo;
    static Supla::Device::StatusLed *pStatusLed;
};
