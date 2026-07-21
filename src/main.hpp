#pragma once

#include <Arduino.h>
#include <BleScanner.hpp>

#include <config.hpp>


#include <SuplaDevice.h>

#include <supla/events.h>

#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>

#include <supla/network/html/button_config_parameters.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/div.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/select_input_parameter.h>
#include <supla/network/html/status_led_parameters.h>
#include <supla/network/html/time_parameters.h>
#include <supla/network/html/wifi_parameters.h>

#include <supla/control/action_trigger.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/control/roller_shutter.h>

#include <supla/device/status_led.h>
#include <supla/device/supla_ca_cert.h>

#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>


#include <devices/BLE_Sensor_Factory.hpp>
#include <html/DeviceConfigurator.hpp>


Supla::ESPWifi wifi;
// Supla::Eeprom eeprom;
Supla::LittleFsConfig configSupla(1024 * 10);

Supla::Device::StatusLed statusLed(GPIO_LED_NUM, GPIO_LED_INV); // inverted state
Supla::Control::Button cfgButton(GPIO_CFG_NUM, GPIO_CFG_PULL, GPIO_CFG_INVERT);

Supla::EspWebServer suplaServer;

Supla::Html::DeviceConfigurator* bleCfg;
BleScanner scanner;



void initHtml();
