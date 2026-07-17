# 本地 HTTP 协议与语义边界

## 启用条件与网络限制

公开固件只提交空值的 `firmware/src/wifi_credentials.example.h`。默认不会创建接入点、不会尝试连接 Wi‑Fi，也不会启动 HTTP。需要本机复现实验时，复制它为 Git 忽略的 `firmware/src/wifi_credentials.h` 并填写自己的可信局域网凭据；只有成功连接后，HTTP 服务才监听设备所在局域网的端口 `80`。凭据文件绝不能提交、截图、粘贴到 Issue 或作为构建证据。

本服务没有 TLS、认证、会话、访问控制、请求签名、速率限制、审计或设备身份校验；只可用于隔离、可信的教学局域网，不能暴露到公网、端口转发、共享热点或不可信网络。

页面和 API 使用同源请求，公开固件没有 `Access-Control-Allow-Origin: *`。这只是移除不需要的宽松 CORS，不构成认证或跨网络安全能力。

公开版不提供 HTTP 网络配置或重置接口。维护本机凭据属于受控的本地文件操作，不应由局域网 GET/POST 请求触发。

## `GET /api/status`

返回当前程序的一次本地状态快照。示例仅展示字段格式，不是设备在线、当前读数或真实硬件证据：

```json
{
  "sensor": "available",
  "lux": 42.7,
  "thresholdLux": 50,
  "autoModeEnabled": false,
  "scope": "trusted_local_network_low_voltage_demo",
  "leds": [
    {"id": 0, "enabled": false, "countdownSeconds": 0, "countdownLeftSeconds": 0}
  ]
}
```

| 字段 | 含义 | 不代表 |
| :-- | :-- | :-- |
| `sensor` | `available` 表示本次读取成功；`unavailable` 表示未初始化或本次读取失败 | 传感器已经校准、持续可靠或设备在线 |
| `lux` | 可用时的本次 BH1750 原始 lux 值；不可用时为 `null` | 经认证照度、节能效果、环境安全或实时监测 |
| `autoModeEnabled` | 当前这次启动中的四路联动自动模式开关 | 每路独立自动策略、无人值守控制或可靠任务执行 |
| `leds[].enabled` | 固件运行态请求写入该 GPIO 的逻辑状态 | 真实 LED 已亮/灭、驱动成功、电源正常或控制送达 |
| `countdownLeftSeconds` | 当前运行态倒计时剩余秒数 | 计时准确、长期可靠或断电恢复 |

## `POST /api/led`

请求体必须是 JSON 对象：

```json
{"id": 0, "enabled": true, "countdownSeconds": 300}
```

| 字段 | 规则 |
| :-- | :-- |
| `id` | 必填整数，范围 `0`–`3`。 |
| `enabled` | 必填布尔值。 |
| `countdownSeconds` | 可选无符号整数，只允许 `0`、`300`、`900`、`1800`、`3600`。设置倒计时会按请求值开启该路。 |

请求体最大 `512` 字节。异步服务器会完整收集 JSON body 后才进行处理；超出上限或不完整/无效数据会拒绝。成功响应例：

```json
{"ok": true, "code": "updated"}
```

HTTP `200` 只表示该 HTTP 处理函数接受了请求；不表示物理负载、网络链路、控制结果、灯光状态或使用者行为已被确认。

## `POST /api/config`

请求体至少提供一个字段：

```json
{"thresholdLux": 50, "autoModeEnabled": true}
```

| 字段 | 规则 |
| :-- | :-- |
| `thresholdLux` | 可选整数，范围 `5`–`300` lux。 |
| `autoModeEnabled` | 可选布尔值。启用时，只有 BH1750 可用且本次读取成功，自动模式才会改变四路输出。 |

自动模式低于阈值时请求开启四路，高于阈值加 `10` lux 时请求关闭四路；位于迟滞区间时保持先前自动输出。自动模式会同时覆盖四路手动状态并清除倒计时。传感器不可用或读数失败时，不改变任何输出。

成功响应同样不代表实体灯光、传感器或网络可靠。错误响应使用 `400`（无效 JSON / 字段 / 范围）或 `413`（请求体超过限制）并附带机器可读 `code`。
