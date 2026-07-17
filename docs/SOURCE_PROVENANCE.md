# 源码来源与权威副本裁决

> 状态日期：2026-07-18

## 只读来源

```text
/home/rongyi/桌面/smart_light_controller
```

来源目录不是 Git 工作树。候选建立前只纳入下列允许公开、可读文本源：

```text
README.md
include/webpage.h
src/config.h
src/main.cpp
platformio.ini
```

建立候选时记录的原始 SHA-256：

| 来源文件 | SHA-256 |
| :-- | :-- |
| `README.md` | `2d667ab6a30f3db639d03de3fd2d3c1ee16b104f7bc07f6848ce5d0e102d47e2` |
| `include/webpage.h` | `1e25da4a99550f435736eee41fdaf0b4fcd6f596adbbd4f8d38d0b1221824a97` |
| `src/config.h` | `af02b53a3cbce50f52bb24643635402b2c9923d002b7c1a93be8ae1d5f7c9556` |
| `src/main.cpp` | `9bdb291954afbd7f746b1415586589523da1816195e2d614bebe986209fa1d36` |
| `platformio.ini` | `117eca1642b187efc9a9e0e697de487a5a786ba8a5d643d0eea663d58a6b80c7` |

## 排除与裁决

以下来源内容不进入公开候选、Git 历史或构建证据：

```text
.pio/       # 本机构建缓存、库副本、对象、ELF、MAP、固件二进制
.vscode/    # 本机 IDE 设置与状态
```

- 桌面原目录是本轮功能来源，始终只读；
- `/home/rongyi/桌面/Hardware Lab-公开仓库/esp32-smart-light-controller` 是唯一允许写入的公开候选；
- 公开候选不会反向覆盖、清理、删除或初始化来源目录；
- 原始 README 中的 `192.168.4.1` 只是 WiFiManager 常见配置入口示例，不复制到公开候选，以避免把未按当前提交复测的接入点行为写成事实；公开候选也不保留 WiFiManager 或 Captive Portal。

## 公开整理差异

公开候选不宣称与原始文件逐字节一致。为把历史项目收口为可审计的教学参考，候选进行了以下必要整理：

1. 将浮动 PlatformIO 平台、`master` 和 Git URL 依赖锁定为 Registry 的明确版本，并统一为当前维护方 `ESP32Async` 的依赖坐标；
2. 删除启动时四路全亮测试，不恢复任一路输出或自动模式；每次启动先请求四路关闭；
3. 不使用上游 `AsyncJson` helper；公开固件自行按确切总长度累积两条 JSON POST，限制请求体、校验分片顺序、显式终止缓冲区，并校验字段、类型、范围和倒计时选项；
4. 将 BH1750 初始化/读取结果保留为运行时语义；不可用时 API 返回 `sensor: "unavailable"`、`lux: null`，自动模式不改变输出；
5. 删除 `Access-Control-Allow-Origin: *`、OPTIONS 路由及无认证 HTTP Wi‑Fi 重置接口；移除 WiFiManager，公开默认没有网络凭据、不创建 AP/Captive Portal、也不启动 HTTP；只有本机忽略的 `wifi_credentials.h` 凭据文件成功连接可信网络后才启用 HTTP；
6. 将浏览器页面改为自包含的系统字体和本地文案，不请求 Google Fonts 或图标 CDN；
7. 补充 MIT、第三方声明、BOM、源码推导接线边界图、状态/协议/验证说明、敏感信息扫描、源码契约与 GitHub Actions。

这些是公开前的安全、可复现性与文档整理，不得被表述为当前实物、网络、照度、自动控制或长期稳定性验证。
