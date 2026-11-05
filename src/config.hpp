#pragma once

#define MAX_DEVICES_COUNT 16
#define MAX_SENSORS_COUNT MAX_DEVICES_COUNT * 3

#define DEVICE_NAME "BLE2SUPLA"

#define BLE_RSSI_MIN -110
#define BLE_RSSI_MAX -35

#ifndef GPIO_LED_NUM
    #define GPIO_LED_NUM 2
#endif
#ifndef GPIO_LED_MODE
    #define GPIO_LED_MODE 1
#endif

#define GPIO_CFG_NUM 0
#define GPIO_CFG_PULL true
#define GPIO_CFG_INVERT true
