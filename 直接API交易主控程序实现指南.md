# 直接API交易主控程序实现指南

## 实现概述

由于代码量较大，我将分步骤实现完整的直接API交易主控程序。当前已创建基础框架 `HedgeArbControlProgram_DirectAPI.cpp`，需要进一步完善。

## 已完成部分

✅ 基础框架结构
✅ 配置数据结构
✅ API初始化和连接
✅ 报价获取逻辑
✅ 交易逻辑框架
✅ 线程管理

## 待完善部分

### 1. GUI界面实现

需要添加以下控件：

#### 参数设置区域
- Edit控件：逻辑品种、API Key、API Secret、交易品种、手数
- CheckBox：使用WebSocket
- Edit控件：交易参数（阈值、目标、止损等）
- Button：保存配置、连接API、启动交易、停止交易

#### 报价显示区域
- ListView：显示A端和B端报价
- 实时更新价差

#### 持仓显示区域
- ListView：显示当前持仓
- 显示总盈亏

#### 交易历史区域
- ListView：显示交易历史

### 2. 配置文件管理

实现配置文件的保存和加载：
- JSON格式配置文件
- 参数验证
- 默认值设置

### 3. 错误处理

- API连接失败处理
- 交易失败处理
- 网络异常处理
- 用户友好的错误提示

### 4. 日志记录

- 交易日志
- 错误日志
- 操作日志
- 日志文件保存

## 快速开始

### 步骤1：编译基础框架

1. 在Visual Studio中创建新的Win32项目
2. 添加以下文件：
   - `HedgeArbControlProgram_DirectAPI.cpp`
   - `ForexAPI/ForexComAPI.cpp`
   - `ForexAPI/HttpClient.cpp`
   - `ForexAPI/ForexComAPI.h`
   - `ForexAPI/HttpClient.h`

3. 配置项目：
   - 包含目录：添加 `ForexAPI` 目录
   - C++标准：C++11或更高
   - 链接库：comctl32.lib

### 步骤2：完善GUI界面

参考 `HedgeArbControlProgramGUI.cpp` 的实现，添加：
- 参数设置对话框或面板
- 报价显示ListView
- 持仓显示ListView
- 交易历史ListView
- 控制按钮

### 步骤3：实现配置文件

创建配置文件管理功能：
- 保存配置到JSON文件
- 从JSON文件加载配置
- 参数验证

### 步骤4：测试

1. 使用测试API Key连接
2. 测试报价获取
3. 测试交易功能（使用模拟账户）

## 关键实现点

### 1. 参数验证

```cpp
bool ValidateConfig(const TradingConfig& config) {
    // 检查逻辑品种是否一致（虽然只有一个，但确保不为空）
    if (config.logicalSymbol.empty()) {
        return false;
    }
    
    // 检查A端和B端配置
    if (config.brokerA.apiKey.empty() || config.brokerB.apiKey.empty()) {
        return false;
    }
    
    // 检查交易参数合理性
    if (config.thresholdPips <= 0 || config.targetPips < 0) {
        return false;
    }
    
    return true;
}
```

### 2. 价差计算

```cpp
double CalculateSpread(const QuoteData& quoteA, const QuoteData& quoteB) {
    // 价差1: Bid_B - Ask_A (做多A，做空B)
    double spread1 = quoteB.bid - quoteA.ask;
    
    // 价差2: Bid_A - Ask_B (做多B，做空A)
    double spread2 = quoteA.bid - quoteB.ask;
    
    // 返回较大的价差
    return (spread1 > spread2) ? spread1 : spread2;
}
```

### 3. 同时开仓

```cpp
bool OpenArbitragePosition() {
    // 计算价差
    double spread = CalculateSpread(g_quoteA, g_quoteB);
    bool useSpread1 = (g_quoteB.bid - g_quoteA.ask) > (g_quoteA.bid - g_quoteB.ask);
    
    bool successA = false, successB = false;
    
    if (useSpread1) {
        // 做多A，做空B
        successA = OpenPosition(g_config.brokerA, g_positionA, 
                               g_config.brokerA.tradeSymbol, 
                               OrderSide::BUY, g_config.brokerA.lots);
        successB = OpenPosition(g_config.brokerB, g_positionB, 
                               g_config.brokerB.tradeSymbol, 
                               OrderSide::SELL, g_config.brokerB.lots);
    } else {
        // 做多B，做空A
        successA = OpenPosition(g_config.brokerA, g_positionA, 
                               g_config.brokerA.tradeSymbol, 
                               OrderSide::SELL, g_config.brokerA.lots);
        successB = OpenPosition(g_config.brokerB, g_positionB, 
                               g_config.brokerB.tradeSymbol, 
                               OrderSide::BUY, g_config.brokerB.lots);
    }
    
    // 如果一边失败，关闭另一边
    if (successA && !successB) {
        ClosePosition(g_config.brokerA, g_positionA);
    } else if (!successA && successB) {
        ClosePosition(g_config.brokerB, g_positionB);
    }
    
    return successA && successB;
}
```

## 下一步工作

1. **完善GUI界面**：添加所有必要的控件和布局
2. **实现配置文件管理**：JSON格式的保存和加载
3. **完善错误处理**：各种异常情况的处理
4. **添加日志功能**：记录所有重要操作
5. **性能优化**：减少延迟，提高响应速度
6. **测试和调试**：全面测试所有功能

## 注意事项

1. **API Key安全**：不要硬编码API Key，使用配置文件或加密存储
2. **网络稳定性**：实现自动重连机制
3. **交易安全**：添加多重确认，防止误操作
4. **数据备份**：定期保存交易记录和配置
5. **测试环境**：先在测试环境充分测试，再使用生产环境

## 相关文件

- `HedgeArbControlProgram_DirectAPI.cpp` - 主程序框架
- `直接API交易主控程序设计.md` - 设计文档
- `ForexAPI/` - API客户端库

