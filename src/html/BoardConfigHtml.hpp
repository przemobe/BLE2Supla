#pragma once

#include <supla/network/web_sender.h>
#include <supla/network/html_element.h>


class BoardConfigHtml : public Supla::HtmlElement
{
public:
    enum LedGpioMode
    {
        LedGpioMode_NonInverted = 0,
        LedGpioMode_Inverted = 1,
        LedGpioMode_COUNT,
    };

    static uint8_t getStatusLedGpio();
    static LedGpioMode getStatusLedGpioMode();

    void send(Supla::WebSender* sender) override;
    bool handleResponse(const char* key, const char* value) override;

    static void makeSuplaStatusLed();
};
