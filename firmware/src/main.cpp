#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <BH1750.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>

#include "config.h"
#if __has_include("wifi_credentials.h")
#include "wifi_credentials.h"
#else
#include "wifi_credentials.example.h"
#endif
#include "webpage.h"

namespace {

AsyncWebServer server(80);
BH1750 lightSensor;
Preferences preferences;

struct LedState {
  bool enabled = false;
  uint32_t countdownSeconds = 0;
  uint32_t countdownLeftSeconds = 0;
};

struct GlobalConfig {
  uint16_t lightThresholdLux = DEFAULT_LIGHT_THRESHOLD_LUX;
  bool autoModeEnabled = DEFAULT_AUTO_MODE;
};

LedState ledStates[LED_CHANNEL_COUNT];
GlobalConfig globalConfig;
bool sensorAvailable = false;
bool autoModeLastOutput = false;
bool autoModeHasOutput = false;
bool httpStarted = false;
unsigned long lastAutoSensorPoll = 0;

void updateLed(const size_t index) {
  if (index >= LED_CHANNEL_COUNT) {
    return;
  }
  digitalWrite(LED_PINS[index], ledStates[index].enabled ? HIGH : LOW);
}

void loadConfig() {
  preferences.begin("smart-light", false);
  globalConfig.lightThresholdLux = preferences.getUShort("threshold", DEFAULT_LIGHT_THRESHOLD_LUX);
  // A reset must begin with all outputs and automation off. The threshold is a
  // non-actuating preference, so it may persist.
  globalConfig.autoModeEnabled = false;
  for (size_t index = 0; index < LED_CHANNEL_COUNT; ++index) {
    ledStates[index].enabled = false;
    ledStates[index].countdownSeconds = 0;
    ledStates[index].countdownLeftSeconds = 0;
    updateLed(index);
  }
  preferences.end();
}

void saveConfig() {
  preferences.begin("smart-light", false);
  preferences.putUShort("threshold", globalConfig.lightThresholdLux);
  preferences.end();
}

void sendJson(AsyncWebServerRequest *request, const int status, JsonDocument &document) {
  String response;
  serializeJson(document, response);
  request->send(status, "application/json", response);
}

void sendResult(AsyncWebServerRequest *request, const int status, const char *code) {
  StaticJsonDocument<128> document;
  document["ok"] = status >= 200 && status < 300;
  document["code"] = code;
  sendJson(request, status, document);
}

bool readLightLux(float &lux) {
  if (!sensorAvailable) {
    return false;
  }
  const float reading = lightSensor.readLightLevel();
  if (reading < 0 || isnan(reading)) {
    sensorAvailable = false;
    return false;
  }
  lux = reading;
  return true;
}

void handleCountdown() {
  static unsigned long lastSecond = 0;
  if (millis() - lastSecond < 1000) {
    return;
  }
  lastSecond = millis();
  for (size_t index = 0; index < LED_CHANNEL_COUNT; ++index) {
    if (ledStates[index].countdownLeftSeconds == 0) {
      continue;
    }
    --ledStates[index].countdownLeftSeconds;
    if (ledStates[index].countdownLeftSeconds == 0) {
      ledStates[index].enabled = false;
      updateLed(index);
    }
  }
}

void handleAutoMode() {
  if (!globalConfig.autoModeEnabled || millis() - lastAutoSensorPoll < AUTO_SENSOR_POLL_INTERVAL_MS) {
    return;
  }
  lastAutoSensorPoll = millis();

  float lux = 0;
  if (!readLightLux(lux)) {
    // A missing or failed BH1750 never changes physical outputs.
    return;
  }

  bool shouldEnable = autoModeHasOutput ? autoModeLastOutput : false;
  if (lux < globalConfig.lightThresholdLux) {
    shouldEnable = true;
  } else if (lux > globalConfig.lightThresholdLux + AUTO_MODE_HYSTERESIS_LUX) {
    shouldEnable = false;
  }

  if (autoModeHasOutput && shouldEnable == autoModeLastOutput) {
    return;
  }

  for (size_t index = 0; index < LED_CHANNEL_COUNT; ++index) {
    ledStates[index].enabled = shouldEnable;
    ledStates[index].countdownSeconds = 0;
    ledStates[index].countdownLeftSeconds = 0;
    updateLed(index);
  }
  autoModeLastOutput = shouldEnable;
  autoModeHasOutput = true;
}

void handleStatus(AsyncWebServerRequest *request) {
  StaticJsonDocument<640> document;
  float lux = 0;
  if (readLightLux(lux)) {
    document["sensor"] = "available";
    document["lux"] = lux;
  } else {
    document["sensor"] = "unavailable";
    document["lux"] = nullptr;
  }
  document["thresholdLux"] = globalConfig.lightThresholdLux;
  document["autoModeEnabled"] = globalConfig.autoModeEnabled;
  document["scope"] = "trusted_local_network_low_voltage_demo";
  JsonArray leds = document.createNestedArray("leds");
  for (size_t index = 0; index < LED_CHANNEL_COUNT; ++index) {
    JsonObject led = leds.createNestedObject();
    led["id"] = index;
    led["enabled"] = ledStates[index].enabled;
    led["countdownSeconds"] = ledStates[index].countdownSeconds;
    led["countdownLeftSeconds"] = ledStates[index].countdownLeftSeconds;
  }
  sendJson(request, 200, document);
}

bool getExactInteger(JsonObject document, const char *name, int &value) {
  JsonVariant field = document[name];
  if (!field.is<int>()) {
    return false;
  }
  value = field.as<int>();
  return true;
}

bool getExactUnsigned(JsonObject document, const char *name, uint32_t &value) {
  JsonVariant field = document[name];
  if (!field.is<uint32_t>()) {
    return false;
  }
  value = field.as<uint32_t>();
  return true;
}

bool parseJsonBody(AsyncWebServerRequest *request, uint8_t *data, const size_t len, const size_t index, const size_t total,
                   DynamicJsonDocument &document) {
  if (total == 0 || total > MAX_REQUEST_BODY_BYTES || index > total || len > total - index) {
    sendResult(request, 413, "body_too_large");
    return false;
  }
  if (index == 0) {
    if (request->_tempObject != nullptr) {
      free(request->_tempObject);
      request->_tempObject = nullptr;
    }
    request->_tempObject = malloc(total + 1);
    if (request->_tempObject == nullptr) {
      sendResult(request, 503, "body_storage_unavailable");
      return false;
    }
  }
  if (request->_tempObject == nullptr) {
    sendResult(request, 400, "invalid_body_sequence");
    return false;
  }
  memcpy(static_cast<uint8_t *>(request->_tempObject) + index, data, len);
  if (index + len != total) {
    return false;
  }

  auto *buffer = static_cast<char *>(request->_tempObject);
  buffer[total] = '\0';
  const DeserializationError error = deserializeJson(document, buffer, total);
  free(request->_tempObject);
  request->_tempObject = nullptr;
  if (error) {
    sendResult(request, 400, "invalid_json");
    return false;
  }
  return true;
}

void handleLedBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  DynamicJsonDocument document(MAX_REQUEST_BODY_BYTES);
  if (!parseJsonBody(request, data, len, index, total, document)) {
    return;
  }
  if (!document.is<JsonObject>()) {
    sendResult(request, 400, "invalid_json");
    return;
  }
  JsonObject object = document.as<JsonObject>();
  int id = 0;
  if (!getExactInteger(object, "id", id) || !object["enabled"].is<bool>()) {
    sendResult(request, 400, "invalid_json");
    return;
  }
  if (id < 0 || static_cast<size_t>(id) >= LED_CHANNEL_COUNT) {
    sendResult(request, 400, "invalid_led_id");
    return;
  }
  uint32_t countdown = 0;
  if (object.containsKey("countdownSeconds")) {
    if (!getExactUnsigned(object, "countdownSeconds", countdown)) {
      sendResult(request, 400, "invalid_countdown");
      return;
    }
    if (countdown != 0 && countdown != 300 && countdown != 900 && countdown != 1800 && countdown != 3600) {
      sendResult(request, 400, "unsupported_countdown");
      return;
    }
  }
  ledStates[id].enabled = object["enabled"].as<bool>();
  ledStates[id].countdownSeconds = countdown;
  ledStates[id].countdownLeftSeconds = countdown;
  updateLed(id);
  sendResult(request, 200, "updated");
}

void handleConfigBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  DynamicJsonDocument document(MAX_REQUEST_BODY_BYTES);
  if (!parseJsonBody(request, data, len, index, total, document)) {
    return;
  }
  if (!document.is<JsonObject>()) {
    sendResult(request, 400, "invalid_json");
    return;
  }
  JsonObject object = document.as<JsonObject>();
  if (!object.containsKey("thresholdLux") && !object.containsKey("autoModeEnabled")) {
    sendResult(request, 400, "missing_config_fields");
    return;
  }
  uint32_t threshold = 0;
  if (object.containsKey("thresholdLux")) {
    if (!getExactUnsigned(object, "thresholdLux", threshold) || threshold < 5 || threshold > 300) {
      sendResult(request, 400, "invalid_threshold");
      return;
    }
  }
  if (object.containsKey("autoModeEnabled") && !object["autoModeEnabled"].is<bool>()) {
    sendResult(request, 400, "invalid_auto_mode");
    return;
  }
  if (object.containsKey("thresholdLux")) {
    globalConfig.lightThresholdLux = static_cast<uint16_t>(threshold);
  }
  if (object.containsKey("autoModeEnabled")) {
    globalConfig.autoModeEnabled = object["autoModeEnabled"].as<bool>();
    autoModeHasOutput = false;
  }
  saveConfig();
  sendResult(request, 200, "updated");
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", CONTROL_PAGE);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/led", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, handleLedBody);
  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, handleConfigBody);
  server.onNotFound([](AsyncWebServerRequest *request) {
    sendResult(request, 404, "not_found");
  });
  server.begin();
  httpStarted = true;
}

void connectTrustedLocalNetwork() {
  if (WIFI_SSID[0] == '\0') {
    Serial.println("[WiFi] no local credentials; HTTP service remains disabled");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    setupWebServer();
    Serial.println("[WiFi] connected; local HTTP service started");
  } else {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WiFi] connection unavailable; HTTP service remains disabled");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  for (const uint8_t pin : LED_PINS) {
    pinMode(pin, OUTPUT);
    // Safe default: no startup light test and no implicit output pulse.
    digitalWrite(pin, LOW);
  }

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sensorAvailable = lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (!sensorAvailable) {
    Serial.println("[BH1750] unavailable; automatic output changes are disabled");
  }

  loadConfig();
  connectTrustedLocalNetwork();
}

void loop() {
  handleCountdown();
  handleAutoMode();
  delay(20);
}
