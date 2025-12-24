# EA通信机制说明

## 📋 通信架构

两个EA（A端和B端）通过**共享内存**进行通信，**共享内存是中心节点**。

### 核心架构

```
                    ┌─────────────────────┐
                    │   共享内存区域       │
                    │  (Shared Memory)    │
                    │  中心节点，所有     │
                    │  进程都访问它       │
                    └──────────┬──────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
                ▼              ▼              ▼
        ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
        │  MT4 A端     │ │  MT4 B端     │ │  总控GUI     │
        │  (EA A)      │ │  (EA B)      │ │  (监控)      │
        └──────┬───────┘ └──────┬───────┘ └──────┬───────┘
               │                │                │
               ▼                ▼                ▼
        ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
        │  DLL A       │ │  DLL B       │ │  直接访问     │
        │  (在MT4的    │ │  (在MT4的    │ │  (C++版本)   │
        │  Libraries)  │ │  Libraries)  │ │  或P/Invoke   │
        └──────────────┘ └──────────────┘ └──────────────┘
```

**关键点**：
- ✅ **共享内存是中心节点**：所有进程都访问同一个共享内存
- ✅ **EA通过DLL访问**：A端EA和B端EA都通过各自的DLL访问共享内存
- ✅ **总控GUI也访问**：用于监控和写入历史记录
- ✅ **不需要总控GUI运行**：两个EA可以直接通信（共享内存独立存在）

---

## 🔄 通信流程

### 1. 报价数据通信

#### A端EA写入报价 → B端EA读取

```
MT4 A端 (EA A)
  └─> OnTick() 触发
      └─> SetGlobalBidAsk("A", bid, ask)
          └─> DLL A.SetQuote("A", bid, ask)
              └─> 写入共享内存 quotes[0]
                  └─> 共享内存已更新
                      ├─> B端EA通过DLL B.GetQuote("A") 读取
                      └─> 总控GUI直接读取 quotes[0] 显示
```

#### B端EA写入报价 → A端EA读取

```
MT4 B端 (EA B)
  └─> OnTick() 触发
      └─> SetGlobalBidAsk("B", bid, ask)
          └─> DLL B.SetQuote("B", bid, ask)
              └─> 写入共享内存 quotes[1]
                  └─> 共享内存已更新
                      ├─> A端EA通过DLL A.GetQuote("B") 读取
                      └─> 总控GUI直接读取 quotes[1] 显示
```

**特点**：
- ✅ **双向通信**：A端和B端都可以写入和读取
- ✅ **实时更新**：每次OnTick都会更新
- ✅ **无中间层**：直接通过共享内存，无延迟

---

### 2. 指令通信（开仓指令）

#### A端EA发送开仓指令 → B端EA接收并执行

```
步骤1：A端EA检测到开仓条件
MT4 A端 (EA A)
  └─> 检测到价差满足条件
      └─> SendCommand("OPEN;A;B")
          └─> SetGlobalCommand("OPEN;A;B")
              └─> DLL A.SetCommand("OPEN", "A", "B")
                  └─> 写入共享内存 command
                      └─> command.code = "OPEN"
                      └─> command.longBroker = "A"
                      └─> command.shortBroker = "B"
                      └─> command.isActive = true

步骤2：B端EA读取指令
MT4 B端 (EA B)
  └─> OnTick() 触发
      └─> ExecCommands()
          └─> GetGlobalCommand()
              └─> DLL B.GetCommand()
                  └─> 读取共享内存 command
                      └─> 检测到 command.code = "OPEN"
                      └─> 检测到 command.shortBroker = "B"（本端）
                          └─> 执行开仓（做空）

步骤3：B端EA发送确认
MT4 B端 (EA B)
  └─> 开仓成功
      └─> SetGlobalAck("B", "SUCCESS")
          └─> DLL B.SetAck("B", "SUCCESS")
              └─> 写入共享内存 acks[1]
                  └─> acks[1].code = "SUCCESS"
                  └─> acks[1].isValid = true

步骤4：A端EA读取确认
MT4 A端 (EA A)
  └─> OnTick() 触发
      └─> CheckOtherSideSucceeded()
          └─> GetGlobalAck("B")
              └─> DLL A.GetAck("B")
                  └─> 读取共享内存 acks[1]
                      └─> 检测到 acks[1].code = "SUCCESS"
                          └─> 确认B端已成功开仓
```

**关键点**：
- ✅ **A端是主控端**：负责发送指令
- ✅ **B端是从控端**：负责接收和执行指令
- ✅ **通过共享内存传递**：指令和确认都通过共享内存
- ✅ **实时通信**：每次OnTick都会检查

---

### 3. 盈亏数据通信

#### A端EA写入盈亏 → B端EA读取

```
MT4 A端 (EA A)
  └─> OnTick() 触发
      └─> 计算当前盈亏
          └─> SetGlobalProfit("A", profitPips, profitUSD)
              └─> DLL A.SetProfit("A", profitPips, profitUSD)
                  └─> 写入共享内存 profits[0]
                      └─> B端EA通过DLL B.GetProfit("A") 读取
```

#### B端EA写入盈亏 → A端EA读取

```
MT4 B端 (EA B)
  └─> OnTick() 触发
      └─> 计算当前盈亏
          └─> SetGlobalProfit("B", profitPips, profitUSD)
              └─> DLL B.SetProfit("B", profitPips, profitUSD)
                  └─> 写入共享内存 profits[1]
                      └─> A端EA通过DLL A.GetProfit("B") 读取
```

---

## 🔧 技术实现

### 共享内存结构

```cpp
struct SharedData {
    // 报价数据
    struct Quote {
        double bid;
        double ask;
        unsigned long long timestamp;
        bool isValid;
    } quotes[2];  // [0]=Broker A, [1]=Broker B
    
    // 盈亏数据
    struct Profit {
        double profitPips;
        double profitUSD;
        unsigned long long timestamp;
        bool isValid;
    } profits[2];
    
    // 指令数据
    struct Command {
        char code[16];      // "OPEN", "CLOSEALL"
        char longBroker[4];  // "A" or "B"
        char shortBroker[4];
        unsigned long long timestamp;
        bool isActive;
    } command;
    
    // 确认数据
    struct Ack {
        char code[16];      // "SUCCESS", "FAILED"
        unsigned long long timestamp;
        bool isValid;
    } acks[2];  // [0]=Broker A, [1]=Broker B
    
    // 同步标志
    volatile long version;  // 每次更新时递增，用于检测数据变化
};
```

### EA端访问方式

#### A端EA（通过DLL A）

```mql4
// 写入报价
SetGlobalBidAsk("A", bid, ask)
  └─> DLL A.SetQuote("A", bid, ask)
      └─> 写入共享内存 quotes[0]

// 读取B端报价
GetGlobalBidAsk("B", bid, ask)
  └─> DLL A.GetQuote("B", bid, ask)
      └─> 读取共享内存 quotes[1]

// 发送指令
SetGlobalCommand("OPEN;A;B")
  └─> DLL A.SetCommand("OPEN", "A", "B")
      └─> 写入共享内存 command

// 读取B端确认
GetGlobalAck("B", ackCode)
  └─> DLL A.GetAck("B", ackCode)
      └─> 读取共享内存 acks[1]
```

#### B端EA（通过DLL B）

```mql4
// 写入报价
SetGlobalBidAsk("B", bid, ask)
  └─> DLL B.SetQuote("B", bid, ask)
      └─> 写入共享内存 quotes[1]

// 读取A端报价
GetGlobalBidAsk("A", bid, ask)
  └─> DLL B.GetQuote("A", bid, ask)
      └─> 读取共享内存 quotes[0]

// 读取指令
GetGlobalCommand()
  └─> DLL B.GetCommand()
      └─> 读取共享内存 command

// 发送确认
SetGlobalAck("B", "SUCCESS")
  └─> DLL B.SetAck("B", "SUCCESS")
      └─> 写入共享内存 acks[1]
```

---

## 📊 数据流向图

### 完整通信流程

```
时间线：T0 → T1 → T2 → T3 → T4

T0: A端EA检测到开仓条件
    └─> SendCommand("OPEN;A;B")
        └─> DLL A.SetCommand()
            └─> 写入共享内存 command

T1: 共享内存已更新（command.isActive = true）
    ├─> B端EA下次OnTick()会读取到
    └─> 总控GUI会显示指令状态

T2: B端EA OnTick()触发
    └─> GetGlobalCommand()
        └─> DLL B.GetCommand()
            └─> 读取共享内存 command
                └─> 检测到指令，执行开仓

T3: B端EA开仓成功
    └─> SetGlobalAck("B", "SUCCESS")
        └─> DLL B.SetAck()
            └─> 写入共享内存 acks[1]

T4: A端EA OnTick()触发
    └─> GetGlobalAck("B")
        └─> DLL A.GetAck()
            └─> 读取共享内存 acks[1]
                └─> 检测到确认，继续流程
```

---

## 🎯 关键特点

### 1. 共享内存是中心节点

- ✅ **所有进程访问同一个共享内存**
- ✅ **共享内存名称**：`HedgeArbEA_SharedMemory_{Symbol}`
- ✅ **例如**：`HedgeArbEA_SharedMemory_GOLD`

### 2. EA通过DLL访问

- ✅ **A端EA**：通过 `HedgeArbSharedMemory.dll`（在MT4 A的Libraries目录）
- ✅ **B端EA**：通过 `HedgeArbSharedMemory.dll`（在MT4 B的Libraries目录）
- ✅ **同一个DLL**：两端使用相同的DLL，访问同一个共享内存

### 3. 不需要总控GUI

- ✅ **EA可以直接通信**：共享内存独立存在，不需要总控GUI运行
- ✅ **总控GUI的作用**：监控、显示、写入历史记录（可选）

### 4. 实时通信

- ✅ **每次OnTick更新**：EA每次收到新报价都会更新共享内存
- ✅ **版本号同步**：使用 `version` 字段检测数据变化
- ✅ **低延迟**：直接内存访问，无网络延迟

---

## 📝 总结

**两端的EA通信方式**：

1. **通过共享内存**：两个EA都访问同一个共享内存区域
2. **通过DLL访问**：A端EA和B端EA都通过各自的DLL访问共享内存
3. **双向通信**：A端可以写入数据供B端读取，B端也可以写入数据供A端读取
4. **实时更新**：每次OnTick都会更新和读取数据
5. **无中间层**：直接通过共享内存，无延迟

**通信路径**：
```
EA A → DLL A → 共享内存 ← DLL B ← EA B
```

**关键**：共享内存是中心节点，所有进程都访问它。

