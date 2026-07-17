from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding='utf-8')


class SourceContracts(unittest.TestCase):
    def test_platform_and_hardware_contract(self):
        ini = read('firmware/platformio.ini')
        config = read('firmware/src/config.h')
        self.assertIn('platform = espressif32@6.13.0', ini)
        self.assertNotIn('#master', ini)
        self.assertNotIn('https://github.com/', ini)
        self.assertNotIn('WiFiManager', ini)
        self.assertIn('ESP32Async/ESPAsyncWebServer@3.6.0', ini)
        self.assertIn('ESP32Async/AsyncTCP@3.4.10', ini)
        for value in [
            'LED_PINS[] = {25, 26, 23, 19}',
            'I2C_SDA_PIN = 21',
            'I2C_SCL_PIN = 22',
            'MAX_REQUEST_BODY_BYTES = 512',
        ]:
            self.assertIn(value, config)

    def test_startup_is_safe_and_does_not_restore_actuating_state(self):
        main = read('firmware/src/main.cpp')
        setup = main[main.index('void setup()'):]
        load = main[main.index('void loadConfig()'):main.index('void saveConfig()')]
        self.assertIn('digitalWrite(pin, LOW)', setup)
        self.assertNotIn('LED test on boot', main)
        self.assertNotIn('digitalWrite(PIN_LED1, HIGH)', main)
        self.assertNotIn('preferences.getBool("autoMode"', main)
        self.assertNotIn('preferences.getBool(key.c_str()', load)
        self.assertIn('globalConfig.autoModeEnabled = false', load)

    def test_json_handlers_accumulate_and_validate_complete_body(self):
        main = read('firmware/src/main.cpp')
        self.assertNotIn('#include <AsyncJson.h>', main)
        self.assertIn('void handleLedBody(', main)
        self.assertIn('void handleConfigBody(', main)
        self.assertIn('server.on("/api/led", HTTP_POST', main)
        self.assertIn('server.on("/api/config", HTTP_POST', main)
        self.assertIn('malloc(total + 1)', main)
        self.assertIn('buffer[total] = \'\\0\'', main)
        self.assertIn('deserializeJson(document, buffer, total)', main)
        self.assertIn('len > total - index', main)
        self.assertIn('free(request->_tempObject)', main)
        self.assertIn('unsupported_countdown', main)
        self.assertIn('missing_config_fields', main)
        self.assertIn('invalid_led_id', main)
        self.assertIn('threshold < 5 || threshold > 300', main)

    def test_network_is_opt_in_with_ignored_local_credentials(self):
        main = read('firmware/src/main.cpp')
        example = read('firmware/src/wifi_credentials.example.h')
        ignore = read('.gitignore')
        self.assertIn('#include <WiFi.h>', main)
        self.assertNotIn('WiFiManager', main)
        self.assertIn('#if __has_include("wifi_credentials.h")', main)
        self.assertIn('if (WIFI_SSID[0] == \'\\0\')', main)
        self.assertIn('WiFi.mode(WIFI_STA)', main)
        self.assertIn('WiFi.mode(WIFI_OFF)', main)
        self.assertIn('WiFi.disconnect(false, false)', main)
        self.assertNotIn('WiFi.disconnect(true, true)', main)
        self.assertIn('setupWebServer();', main)
        self.assertIn('#define WIFI_SSID ""', example)
        self.assertIn('#define WIFI_PASSWORD ""', example)
        self.assertIn('firmware/src/wifi_credentials.h', ignore)

    def test_sensor_unavailable_has_null_semantics_and_does_not_control_outputs(self):
        main = read('firmware/src/main.cpp')
        auto = main[main.index('void handleAutoMode()'):main.index('void handleStatus(')]
        status = main[main.index('void handleStatus('):main.index('bool getExactInteger(')]
        self.assertIn('bool sensorAvailable = false', main)
        self.assertIn('if (!sensorAvailable)', main)
        self.assertIn('sensorAvailable = false', main)
        self.assertIn('if (!readLightLux(lux))', auto)
        self.assertIn('return;', auto)
        self.assertIn('document["sensor"] = "unavailable"', status)
        self.assertIn('document["lux"] = nullptr', status)

    def test_no_wide_cors_or_network_reset_endpoint(self):
        main = read('firmware/src/main.cpp')
        page = read('firmware/include/webpage.h')
        for forbidden in ['Access-Control-Allow-Origin', 'HTTP_OPTIONS', '"/api/reset"', 'resetSettings()', 'fonts.googleapis.com', 'cdnjs.cloudflare.com']:
            self.assertNotIn(forbidden, main + page)
        self.assertIn('server.onNotFound', main)
        self.assertIn('"not_found"', main)

    def test_local_page_uses_explicit_unavailable_and_no_fake_online_state(self):
        page = read('firmware/include/webpage.h')
        self.assertIn("data.sensor!=='available'", page)
        self.assertIn('不表示设备在线', page)
        self.assertNotIn('SYSTEM ONLINE', page)
        self.assertNotIn('fonts.googleapis.com', page)

    def test_docs_keep_build_and_hardware_evidence_separate(self):
        readme = read('README.md')
        verification = read('docs/VERIFICATION.md')
        self.assertIn('当前真机复测 | **未执行。**', readme)
        self.assertIn('构建成功不证明', readme)
        self.assertIn('exact-HEAD CI', readme)
        self.assertIn('不会修改桌面原工程', verification)

    def test_ci_does_not_claim_a_partial_hash_lock(self):
        workflow = read('.github/workflows/validate.yml')
        self.assertIn('platformio==6.1.19', workflow)
        self.assertNotIn('--require-hashes', workflow)


if __name__ == '__main__':
    unittest.main()
