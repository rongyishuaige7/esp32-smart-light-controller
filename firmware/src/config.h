#ifndef SMART_LIGHT_CONFIG_H
#define SMART_LIGHT_CONFIG_H

#include <Arduino.h>

// Four low-voltage LED control outputs. These pins are source-code facts only;
// verify the actual driver circuit, output polarity, current limit and common
// ground before wiring hardware.
constexpr uint8_t LED_PINS[] = {25, 26, 23, 19};
constexpr size_t LED_CHANNEL_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// Optional BH1750 I2C light sensor.
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;

constexpr uint16_t DEFAULT_LIGHT_THRESHOLD_LUX = 50;
constexpr bool DEFAULT_AUTO_MODE = false;
constexpr uint16_t AUTO_MODE_HYSTERESIS_LUX = 10;

constexpr size_t MAX_REQUEST_BODY_BYTES = 512;
constexpr uint32_t AUTO_SENSOR_POLL_INTERVAL_MS = 500;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

#endif
