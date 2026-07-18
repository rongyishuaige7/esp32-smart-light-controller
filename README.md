# 基于ESP32的多路智能照明控制系统

> **教学原型 / 历史项目。** 基于 ESP32、四路低压 LED、可选 BH1750 与可选本地 HTTP 页面的多路照明控制参考。

它演示四路手动 GPIO 控制、五种倒计时选项，以及基于可选光照输入的四路联动自动逻辑。它不是市电照明、智能家居成品、网络安全设备、可靠远程控制系统或可直接长期部署的产品。

## 功能范围

```text
可选 BH1750 ─ I²C ─┐
                    ├─ ESP32 ─ 四路低压 LED 逻辑输出
可信局域网浏览器 ─ HTTP ┘          │
                                     ├─ 手动单路控制
                                     ├─ 倒计时关闭
                                     └─ 可选光照触发的四路联动自动模式
```

- 四路低压 LED 逻辑输出：GPIO25、GPIO26、GPIO23、GPIO19；
- 可选 BH1750：SDA GPIO21、SCL GPIO22；
- 同源本地 Web 页面与 `GET /api/status`、`POST /api/led`、`POST /api/config`；
- 倒计时只允许 `0`、`5`、`15`、`30`、`60` 分钟；
- 自动模式低于阈值时请求开启四路，高于阈值 + 10 lux 时请求关闭四路；
- BH1750 未初始化或读取失败时，状态返回 `sensor: "unavailable"`、`lux: null`，自动模式不改变输出；
- 自动模式会同时覆盖四路手动状态并清除倒计时，当前**不是**每路独立自动控制。

上面是当前源码行为，不是当前设备的实时或已复测状态。

## 接线与电气边界

| 模块 / 信号 | 固件接口 | 关键边界 |
| :-- | :-- | :-- |
| LED 通道 1–4 | GPIO25、GPIO26、GPIO23、GPIO19 | 高电平是固件请求开启。实际极性、驱动、限流、电流、供电和共地均待实物确认。 |
| BH1750（可选） | SDA GPIO21、SCL GPIO22 | 地址、上拉、电压、模块型号和读数都待实物确认。 |
| 可选本机局域网连接 | `wifi_credentials.h`（本机忽略文件） | 仅在本机填入可信网络凭据并连接成功时启动 HTTP 80；默认公开固件不启动 Wi‑Fi/HTTP。实际连接、重连和 URL 没有按当前 commit 复测。 |

完整 BOM 和安全注意事项见 [HARDWARE.md](HARDWARE.md)。`hardware/wiring-diagram.svg` 是按源码推导的接口图，**不是**原理图、PCB、实测接线或电气认证。

请先断电接线，并确认低压电源、公共地、每路 LED 极性、限流/恒流驱动和最大电流。ESP32 GPIO 只是 3.3 V 逻辑输出，不能替代功率电源或直接驱动未知/大电流负载。不得把本项目用于市电、继电器、公共/应急照明、安防、消防、医疗、生产控制或无人值守场景。

## 可选网络与本地控制边界

公开默认固件的 `wifi_credentials.example.h` 是空值：不会创建接入点、不会进行配网，也不会启动 Wi‑Fi 或 HTTP。需要复现实验时，复制该文件为 Git 忽略的 `firmware/src/wifi_credentials.h`，只填入自己**可信本地测试网络**的凭据；仅在连接成功后，HTTP 服务才监听设备所在局域网的端口 `80`。不要提交、截图、回显或分享该本机文件。

可选 HTTP API 没有 TLS、身份认证、权限模型、会话、请求签名、速率限制、审计或设备身份校验。仅可在隔离可信局域网中使用；禁止公网暴露、端口转发、共享热点或不可信网络。

公开版页面和 API 使用同源请求，已移除 `Access-Control-Allow-Origin: *`；这不表示接口已经认证或可安全跨网络使用。公开版也不提供 HTTP Wi‑Fi 重置接口，避免无认证请求清除设备网络配置。

接口字段、错误语义和示例见 [docs/PROTOCOL.md](docs/PROTOCOL.md)。HTTP `200` 只表示一次处理函数接受请求，不表示实体 LED 已动作、控制送达、网络可靠、设备在线或有人处理。

本机凭据文件、ESP32 NVS 或你的网络环境都可能包含 Wi‑Fi 信息。不要公开本机凭据文件、NVS 导出、串口日志、截图、视频、SSID、密码、私网 IP、MAC、二维码或网络拓扑。完整边界见 [SECURITY.md](SECURITY.md)。

## 构建

```bash
cd firmware
pio run
```

固定构建目标：PlatformIO Core `6.1.19`、`espressif32@6.13.0`、`esp32dev`。依赖版本已在 `firmware/platformio.ini` 固定。首次发布前以本仓实际 `scripts/verify.sh` 和最终 GitHub Actions exact-HEAD 结果为准；不要引用原始工程 `.pio/` 缓存作为本仓构建证据。

### 一键门禁

```bash
bash scripts/verify.sh
```

脚本会导出候选到临时目录，运行敏感信息/结构检查、无硬件源码契约与 PlatformIO 构建。它不会修改原工程、烧录硬件、连接真实 Wi‑Fi、写入真实凭据、清除 NVS 或证明真机行为。

## 许可证、第三方与学习使用

- Rongyi 原创的公开整理固件、文档、脚本和接线边界图以 [MIT License](LICENSE) 公开；
- 依赖版本、来源和许可证入口见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)；
- 本项目用于学习、实验、课程参考和二次开发。请保留来源说明，不要把它直接包装为个人课程设计、毕业设计、竞赛或商业产品成果；
- 使用者自行承担硬件、电气、网络、适用性和安全验证责任。

## 更多资料

- [硬件与接线边界](HARDWARE.md)
-
- [来源与权威副本裁决](docs/SOURCE_PROVENANCE.md)
- [本地 HTTP 协议](docs/PROTOCOL.md)
- [验证说明](docs/VERIFICATION.md)
- [GitHub 元数据](docs/GITHUB_METADATA.md)
- [Hardware Lab 索引卡片](docs/HARDWARE_LAB_CARD.md)
- [Hardware Lab](https://github.com/rongyishuaige7/hardware-lab)
