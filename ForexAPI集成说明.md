# ForexAPI 与主控程序集成说明

## 概述

本集成方案将 ForexAPI 连接到主控程序，使主控程序可以通过 Forex.com API 获取实时报价，并将报价写入共享内存，供 MT4 EA 使用。

## 架构设计

```
┌──────────────┐
│  Forex.com   │
│    API       │
└──────┬───────┘
       │ WebSocket/REST API
       ▼
┌──────────────┐
│  ForexAPI    │
│   Client     │
└──────┬───────┘
       │ 获取报价
       ▼
┌──────────────┐         ┌─────────────┐
│  主控程序    │ ◄──────► │  共享内存    │ ◄──────► │  MT4 EA   │
│ (集成版)     │ 写入报价 │              │ 读取报价 │            │
└──────────────┘         └─────────────┘         └─────────────┘
```

## 功能特性

1. **双模式支持**：
   - **REST API 模式**：定期轮询获取报价（默认）
   - **WebSocket 模式**：实时推送报价（延迟更低）

2. **自动写入共享内存**：
   - ForexAPI 获取的报价自动写入共享内存的 Broker A 位置
   - EA 可以像读取 MT4 报价一样读取 ForexAPI 报价

3. **向后兼容**：
   - 如果不启用 ForexAPI，主控程序仍使用 MT4 报价
   - 可以同时使用 ForexAPI（Broker A）和 MT4（Broker B）

## 编译说明

### 1. 添加文件到项目

在 Visual Studio 项目中添加以下文件：

**源文件：**
- `HedgeArbControlProgram_WithForexAPI.cpp`（新文件）
- `ForexAPI/ForexComAPI.cpp`
- `ForexAPI/HttpClient.cpp`

**头文件：**
- `ForexAPI/ForexComAPI.h`
- `ForexAPI/HttpClient.h`
- `HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h`

### 2. 配置项目

1. **包含目录**：
   - 添加 `ForexAPI` 目录到包含路径
   - 添加 `HedgeArbEA/HedgeArbSharedMemory_New` 目录到包含路径

2. **链接库**：
   - WinHTTP（Windows SDK 自带，无需额外链接）

3. **C++ 标准**：
   - 使用 C++11 或更高版本（需要支持 `std::thread`）

### 3. 编译

在 Visual Studio 中编译项目，生成 `HedgeArbControlProgram_WithForexAPI.exe`

## 使用方法

### 基本用法（不使用 ForexAPI）

```bash
HedgeArbControlProgram_WithForexAPI.exe -s GOLD
```

### 使用 ForexAPI（REST API 模式）

```bash
HedgeArbControlProgram_WithForexAPI.exe ^
    -s GOLD ^
    --use-forex-api ^
    --forex-key YOUR_API_KEY ^
    --forex-secret YOUR_API_SECRET ^
    --forex-symbol XAUUSD
```

### 使用 ForexAPI（WebSocket 模式）

```bash
HedgeArbControlProgram_WithForexAPI.exe ^
    -s GOLD ^
    --use-forex-api ^
    --forex-key YOUR_API_KEY ^
    --forex-secret YOUR_API_SECRET ^
    --forex-symbol XAUUSD ^
    --use-websocket
```

## 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-s <symbol>` | MT4 品种名称 | GOLD |
| `-d <directory>` | 数据文件目录 | D:\mt4\test\ |
| `--use-forex-api` | 启用 ForexAPI | 否 |
| `--forex-key <key>` | ForexAPI API Key | （必需） |
| `--forex-secret <secret>` | ForexAPI API Secret | （必需） |
| `--forex-symbol <symbol>` | ForexAPI 品种名称 | XAUUSD |
| `--use-websocket` | 使用 WebSocket | 否（使用 REST API） |
| `-h, --help` | 显示帮助 | - |

## 工作流程

### 1. 启动主控程序

```bash
HedgeArbControlProgram_WithForexAPI.exe --use-forex-api --forex-key KEY --forex-secret SECRET
```

### 2. 主控程序初始化

1. 初始化共享内存
2. 初始化 ForexAPI 客户端
3. 如果使用 WebSocket，连接并订阅报价
4. 启动报价获取线程

### 3. 报价更新流程

**REST API 模式：**
```
ForexAPI 线程 → 每 100ms 调用 GetQuote() → 写入共享内存 → EA 读取
```

**WebSocket 模式：**
```
ForexAPI 线程 → 接收实时报价推送 → 写入共享内存 → EA 读取
```

### 4. EA 使用报价

EA 通过共享内存读取报价，无需知道报价来源（MT4 或 ForexAPI）：

```cpp
// EA 代码（无需修改）
double bidA, askA;
GetQuote("A", &bidA, &askA);  // 从共享内存读取（可能是 ForexAPI 报价）
```

## 配置示例

### 场景 1：ForexAPI 作为 Broker A，MT4 作为 Broker B

```bash
# 主控程序使用 ForexAPI 获取 Broker A 报价
HedgeArbControlProgram_WithForexAPI.exe ^
    -s GOLD ^
    --use-forex-api ^
    --forex-key KEY ^
    --forex-secret SECRET

# MT4 A 端：不加载 EA（或加载但不更新报价）
# MT4 B 端：加载 EA，正常更新报价
```

### 场景 2：ForexAPI 作为两个经纪商（完全替代 MT4 报价）

需要修改代码，让 ForexAPI 同时更新 Broker A 和 Broker B 的报价。

## 注意事项

1. **API Key 安全**：
   - 不要将 API Key 和 Secret 硬编码在代码中
   - 使用环境变量或配置文件
   - 不要提交到版本控制系统

2. **网络连接**：
   - 确保网络连接稳定
   - ForexAPI 连接失败时，主控程序会回退到 MT4 报价模式

3. **报价延迟**：
   - WebSocket 模式延迟更低（推荐用于实盘交易）
   - REST API 模式有轮询延迟（适合测试）

4. **品种名称映射**：
   - MT4 品种名称（如 "GOLD"）可能与 ForexAPI 品种名称（如 "XAUUSD"）不同
   - 使用 `--forex-symbol` 参数指定 ForexAPI 品种名称

## 故障排查

### 问题 1：ForexAPI 初始化失败

**症状**：看到 "ForexAPI 初始化失败" 错误

**解决方法**：
- 检查 API Key 和 Secret 是否正确
- 检查网络连接
- 查看错误信息：`g_forexClient->GetLastError()`

### 问题 2：收不到报价

**症状**：共享内存中 Broker A 的报价始终无效

**解决方法**：
- 检查 ForexAPI 连接状态
- 确认品种名称是否正确（`--forex-symbol`）
- 查看主控程序日志

### 问题 3：WebSocket 连接失败

**症状**：WebSocket 模式无法连接

**解决方法**：
- 检查是否需要 API Key（某些服务需要）
- 尝试使用 REST API 模式（去掉 `--use-websocket`）
- 查看错误信息

## 下一步优化

1. **配置文件支持**：
   - 从 JSON 文件读取配置，而不是命令行参数

2. **多品种支持**：
   - 同时订阅多个品种的报价

3. **自动重连**：
   - WebSocket 断开时自动重连

4. **报价验证**：
   - 验证报价合理性（价格跳跃检测）

5. **性能监控**：
   - 统计报价延迟和更新频率

## 相关文档

- [ForexAPI README.md](./ForexAPI/README.md) - ForexAPI 使用说明
- [共享内存通信实现说明.md](./共享内存通信实现说明.md) - 共享内存通信说明
- [总控程序通信方案设计.md](./总控程序通信方案设计.md) - 总控程序架构设计

