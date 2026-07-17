# 第三方依赖与声明

本仓库未复制第三方库源码。固件由 PlatformIO 在构建时获取下列固定版本；它们仍分别受上游许可证、NOTICE、商标和分发条款约束。使用、再分发或发布二进制前，请以自己的精确构建环境和上游条款为准。

| 组件 | 固定版本 | 用途 | 上游 / 许可证入口 |
| :-- | :-- | :-- | :-- |
| PlatformIO espressif32 | `6.13.0` | ESP32 Arduino 构建平台 | [PlatformIO Registry](https://registry.platformio.org/platforms/platformio/espressif32) |
| Arduino-ESP32 | 由平台锁定 | Arduino 框架、Wi‑Fi、NVS、I²C | [Espressif / Arduino-ESP32](https://github.com/espressif/arduino-esp32) · LGPL-2.1-or-later |
| ESPAsyncWebServer | `3.6.0` | 本地异步 HTTP 服务 | [ESP32Async/ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) · LGPL-3.0 |
| AsyncTCP | `3.4.10` | ESP32 异步 TCP 支持 | [ESP32Async/AsyncTCP](https://github.com/ESP32Async/AsyncTCP) · LGPL-3.0 |
| BH1750 | `1.3.0` | 可选 BH1750 光照传感器读取 | [claws/BH1750](https://github.com/claws/BH1750) · MIT |
| ArduinoJson | `6.21.6` | 本地 JSON 请求与响应 | [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) · MIT |

本仓中 Rongyi 原创的公开整理固件、文档、脚本和源码推导接线图适用 [MIT License](LICENSE)。这不替代上游组件的许可证，也不代表 LED、传感器、开发板、Wi‑Fi 或其他硬件资料可被无条件再分发。
