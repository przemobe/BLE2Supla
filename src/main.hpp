#pragma once

#include <Arduino.h>

#include "config.hpp"

#include <SuplaDevice.h>

#include <supla/events.h>

#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>

#include <supla/control/button.h>

#include <supla/device/supla_ca_cert.h>

#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>

void initHtml();
