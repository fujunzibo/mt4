# GUI数据传递问题排查指南

## 🔍 问题：A/B端数据传不到主控GUI

### 可能原因和解决方案

---

## 1️⃣ 共享内存连接问题

### 症状
- GUI状态栏显示 "Shared Memory: Not Connected"
- 报价表格显示 "N/A"

### 检查步骤

#### 1.1 确认EA是否已启动并初始化共享内存

**检查EA日志（MT4日志窗口）：**
```
[InitSharedMemory] Shared memory initialized successfully. Symbol=GOLD
```

**如果看到错误信息：**
```
[InitSharedMemory] Failed to initialize shared memory
```
→ EA没有成功创建共享内存，需要检查EA参数和DLL部署

#### 1.2 确认符号名称是否匹配

**GUI默认符号：** `GOLD`

**EA使用的符号：** 检查EA输入参数 `InpSymbol`

**共享内存名称格式：** `HedgeArbEA_SharedMemory_{Symbol}`

**示例：**
- EA使用 `InpSymbol=GOLD` → 共享内存名称：`HedgeArbEA_SharedMemory_GOLD`
- GUI默认 `GOLD` → 尝试连接：`HedgeArbEA_SharedMemory_GOLD`
- ✅ **匹配** → 可以连接
- ❌ **不匹配** → 无法连接

**解决方案：**
1. 确保EA的 `InpSymbol` 参数与GUI使用的符号一致
2. 或者通过命令行参数启动GUI：`HedgeArbControlProgramGUI.exe GOLD`

#### 1.3 检查共享内存是否真的存在

**使用DebugView查看DLL日志：**
```
[HedgeArbDLL] [InitSharedMemory] Using mapName: 'HedgeArbEA_SharedMemory_GOLD'
[HedgeArbDLL] [SetQuote] sym=GOLD brokerId='A' bid=2650.50 ask=2651.00
```

**如果没有看到这些日志：**
→ EA可能没有调用DLL函数，或者DLL没有正确部署

---

## 2️⃣ EA数据写入问题

### 症状
- GUI显示 "Shared Memory: Connected"
- 但报价表格仍然显示 "N/A"

### 检查步骤

#### 2.1 确认EA是否在调用SetQuote

**检查EA日志：**
```
[SetGlobalBidAsk] Setting for A - Bid=2650.50 Ask=2651.00
```

**如果看不到这些日志：**
→ EA可能没有在OnTick中调用SetQuote

#### 2.2 检查EA的OnTick函数

**EA应该定期调用：**
```mql4
SetGlobalBidAsk("A", bid, ask);  // A端EA
SetGlobalBidAsk("B", bid, ask);  // B端EA
```

**检查EA代码中的调用频率：**
- 应该在每个Tick或定时器中调用
- 如果使用定时器，检查 `InpCheckIntervalMs` 参数

#### 2.3 检查数据是否过期

**GUI有5秒数据过期检查：**
```cpp
const unsigned long long DATA_EXPIRY_MS = 5000;  // 5秒
```

**如果EA超过5秒没有更新数据：**
→ GUI会认为数据过期，显示 "N/A"

**解决方案：**
- 确保EA定期更新报价（至少每5秒一次）
- 检查EA是否正常运行（没有挂起或错误）

---

## 3️⃣ 文件数据读取问题（持仓和历史）

### 症状
- 报价表格有数据
- 但持仓和历史表格为空

### 检查步骤

#### 3.1 确认文件路径

**GUI使用的路径：**
```
C:\Users\{用户名}\AppData\Roaming\MetaQuotes\Terminal\Common\Files\
```

**EA写入的文件：**
- `ArbState_Positions_{Symbol}_A.csv`
- `ArbState_Positions_{Symbol}_B.csv`
- `ArbState_History_{Symbol}_A.csv`
- `ArbState_History_{Symbol}_B.csv`

**检查文件是否存在：**
1. 打开文件资源管理器
2. 导航到上述路径
3. 查找以 `ArbState_` 开头的CSV文件

#### 3.2 确认EA是否在写入文件

**检查EA日志：**
- EA应该定期调用 `WriteMyPositions()` 和 `WriteTradeHistory()`
- 写入频率：每5次Tick一次（约每250ms）

**如果文件不存在：**
→ EA可能没有写入文件，或者文件路径不对

#### 3.3 检查文件格式

**持仓文件格式：**
```
Ticket;Type;Lots;OpenPrice;CurrentPrice;Profit;Swap;Commission;Time
123456;BUY;0.1;2650.50;2651.00;5.00;0.00;0.00;2025.12.24 12:00
```

**历史文件格式：**
```
Time;Action;Ticket;Type;Lots;OpenPrice;ClosePrice;Profit;Comment
2025.12.24 12:00:00;CLOSE;123456;BUY;0.1;2650.50;2651.00;5.00;
```

**如果文件格式不对：**
→ GUI无法正确解析数据

---

## 4️⃣ 诊断工具和方法

### 4.1 使用DebugView查看DLL日志

1. 下载并运行 DebugView（Sysinternals工具）
2. 启用 "Capture Global Win32" 和 "Capture Win32"
3. 查看以 `[HedgeArbDLL]` 开头的日志

**关键日志：**
```
[HedgeArbDLL] [InitSharedMemory] Using mapName: 'HedgeArbEA_SharedMemory_GOLD'
[HedgeArbDLL] [SetQuote] sym=GOLD brokerId='A' bid=2650.50 ask=2651.00
[HedgeArbDLL] [SetQuote] sym=GOLD brokerId='B' bid=2650.60 ask=2651.10
```

**如果没有这些日志：**
→ DLL没有被调用，或者日志输出被禁用

### 4.2 检查GUI状态栏信息

**GUI状态栏显示：**
```
Symbol: GOLD | Shared Memory: Connected | Version: 12345
```

**如果显示 "Not Connected"：**
→ 共享内存连接失败，检查符号名称和EA是否运行

### 4.3 检查EA日志

**在MT4中查看EA日志：**
- 工具 → 选项 → 专家顾问 → 启用日志
- 查看MT4日志窗口

**关键日志：**
```
[InitSharedMemory] Shared memory initialized successfully. Symbol=GOLD
[SetGlobalBidAsk] Setting for A - Bid=2650.50 Ask=2651.00
```

---

## 5️⃣ 常见问题快速检查清单

### ✅ 检查清单

- [ ] **EA是否在运行？**
  - 检查MT4图表上EA是否显示为绿色（运行中）
  - 检查EA日志是否有错误信息

- [ ] **DLL是否已部署？**
  - 检查 `MQL4\Libraries\HedgeArbSharedMemory.dll` 是否存在
  - 检查DLL文件时间戳是否是最新的

- [ ] **符号名称是否匹配？**
  - EA的 `InpSymbol` = GUI使用的符号
  - 共享内存名称：`HedgeArbEA_SharedMemory_{Symbol}`

- [ ] **EA是否在写入数据？**
  - 检查DebugView是否有 `[SetQuote]` 日志
  - 检查EA日志是否有 `[SetGlobalBidAsk]` 日志

- [ ] **数据是否过期？**
  - GUI每5秒检查一次数据有效性
  - 确保EA至少每5秒更新一次报价

- [ ] **文件路径是否正确？**
  - 检查MT4 Common Files目录是否存在
  - 检查CSV文件是否在该目录中

---

## 6️⃣ 解决方案总结

### 如果共享内存连接失败：

1. **确认EA已启动并初始化共享内存**
   - 检查EA日志：`[InitSharedMemory] Shared memory initialized successfully`
   - 检查DebugView：`[HedgeArbDLL] [InitSharedMemory] Using mapName: ...`

2. **确认符号名称匹配**
   - EA的 `InpSymbol` 必须与GUI使用的符号一致
   - 默认都是 `GOLD`

3. **重启EA和GUI**
   - 先启动EA，等待EA初始化共享内存
   - 再启动GUI，GUI会自动连接

### 如果连接成功但数据为空：

1. **确认EA在写入数据**
   - 检查DebugView：`[HedgeArbDLL] [SetQuote] ...`
   - 检查EA日志：`[SetGlobalBidAsk] ...`

2. **检查数据是否过期**
   - GUI每5秒检查一次
   - 确保EA定期更新（至少每5秒一次）

3. **检查数据有效性**
   - `bid > 0 && ask > 0 && ask > bid`
   - 如果数据无效，GUI不会显示

### 如果持仓/历史数据为空：

1. **检查文件是否存在**
   - 导航到MT4 Common Files目录
   - 查找 `ArbState_Positions_*.csv` 和 `ArbState_History_*.csv`

2. **确认EA在写入文件**
   - 检查EA代码中的 `WriteMyPositions()` 和 `WriteTradeHistory()` 调用
   - 检查写入频率（每5次Tick一次）

3. **检查文件格式**
   - 确保文件是CSV格式，使用分号（;）分隔
   - 确保第一行是表头

---

## 7️⃣ 调试建议

### 增强GUI日志输出

可以在GUI代码中添加更多调试信息：

```cpp
void UpdateQuoteDisplay()
{
    if (!g_pSharedData)
    {
        // 输出调试信息
        char debugMsg[256];
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[GUI] UpdateQuoteDisplay: g_pSharedData is NULL");
        OutputDebugStringA(debugMsg);
        return;
    }
    
    // 检查数据有效性
    if (!g_pSharedData->quotes[0].isValid)
    {
        OutputDebugStringA("[GUI] Broker A quote is not valid");
    }
    // ...
}
```

### 检查共享内存内容

可以使用工具（如Process Explorer）查看共享内存：
- 查找名称：`HedgeArbEA_SharedMemory_GOLD`
- 检查是否被EA进程创建

---

## 📞 如果问题仍然存在

请提供以下信息：

1. **GUI状态栏显示的内容**
2. **DebugView中的DLL日志**（特别是 `[InitSharedMemory]` 和 `[SetQuote]`）
3. **EA日志**（特别是 `[InitSharedMemory]` 和 `[SetGlobalBidAsk]`）
4. **EA的输入参数**（特别是 `InpSymbol`）
5. **文件是否存在**（MT4 Common Files目录中的CSV文件）



