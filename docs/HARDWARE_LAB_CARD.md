# Hardware Lab 索引卡片草案

## 名称

```text
基于ESP32的多路智能照明控制系统
```

## 摘要

```text
基于 ESP32、四路低压 LED、可选 BH1750 与可选本机局域网 Web 页面，演示手动多路控制、倒计时和四路联动的环境亮度自动逻辑。
```

## 平台

```text
ESP32 · Arduino · PlatformIO · ESPAsyncWebServer · BH1750 · 低压 LED · 可选本地 HTTP
```

## 真实状态口径

```text
源码来源已确认；公开版本采用安全启动、分片 JSON 请求、传感器不可用语义、默认无凭据不开 Wi‑Fi/HTTP、无危险 HTTP 重置接口和依赖锁定。
```

## 公开范围与边界

```text
当前未公开实物照片、演示视频、原理图、PCB、Gerber 或制造文件；公开固件、BOM、源码推导接线边界图、来源、协议、安全与验证说明。HTTP 无 TLS、认证或权限模型，仅限隔离可信局域网与低压 LED 教学负载；不适用于市电、公共/应急照明、安防或无人值守。
```

填入 Hardware Lab 前，必须把 `head_sha`、固定 Actions URL、构建证据与最终公开仓 exact HEAD 同步；线上 CI 未完成前不得写为构建已通过。
