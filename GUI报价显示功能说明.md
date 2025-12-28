# GUI报价显示功能说明

## ✅ 已实现的功能

GUI现在可以实时显示经纪商平台的报价数据，包括：

### 1. 报价数据表格

GUI使用`ListView`控件显示以下信息：

| Broker | Bid | Ask | Spread | Last Update |
|--------|-----|-----|--------|-------------|
| Broker A | 2650.50000 | 2651.00000 | 0.50000 | 50 ms ago |
| Broker B | 2650.60000 | 2651.10000 | 0.50000 | 45 ms ago |
| Spread (B-A) | 0.10000 | -0.10000 | 0.10000 | 45 ms ago |
| Summary | 2650.55000 | 2651.05000 | 0.50000 | Live |

### 2. 显示的数据项

#### Broker A / Broker B行：
- **Bid**：买入价
- **Ask**：卖出价
- **Spread**：点差（Ask - Bid）
- **Last Update**：数据更新时间（毫秒前）

#### Spread (B-A)行：
- **Bid列**：`Bid_B - Ask_A`（价差1，用于判断开仓方向）
- **Ask列**：`Bid_A - Ask_B`（价差2，用于判断开仓方向）
- **Spread列**：最大价差（`max(diff1, diff2)`）
- **Last Update**：最新数据的时间

#### Summary行：
- **Bid列**：平均买入价（`(Bid_A + Bid_B) / 2`）
- **Ask列**：平均卖出价（`(Ask_A + Ask_B) / 2`）
- **Spread列**：平均点差（`((Ask_A - Bid_A) + (Ask_B - Bid_B)) / 2`）
- **Last Update**：显示"Live"（实时数据）

---

## 🔧 技术实现

### 数据来源

GUI直接从**共享内存**读取数据：
```cpp
// 读取Broker A报价
if (g_pSharedData->quotes[0].isValid && !IsDataExpired(g_pSharedData->quotes[0].timestamp))
{
    bidA = g_pSharedData->quotes[0].bid;
    askA = g_pSharedData->quotes[0].ask;
    validA = true;
}

// 读取Broker B报价
if (g_pSharedData->quotes[1].isValid && !IsDataExpired(g_pSharedData->quotes[1].timestamp))
{
    bidB = g_pSharedData->quotes[1].bid;
    askB = g_pSharedData->quotes[1].ask;
    validB = true;
}
```

### 更新频率

- **更新间隔**：每100毫秒（`TIMER_INTERVAL_MS = 100`）
- **数据有效性检查**：数据超过5秒未更新则显示"N/A"

### 窗口大小

- **默认大小**：800x400像素
- **列宽自适应**：窗口大小改变时，列宽自动调整

---

## 📋 使用说明

### 1. 启动GUI

```
1. 编译生成 HedgeArbControlProgramGUI.exe
2. 双击运行（或从命令行运行，可指定symbol参数）
3. GUI会自动连接共享内存
```

### 2. 命令行参数（可选）

```
HedgeArbControlProgramGUI.exe GOLD
```

如果不指定参数，默认使用`GOLD`。

### 3. 查看数据

- GUI会自动从共享内存读取报价数据
- 如果EA未启动或数据过期，显示"N/A"
- 状态栏显示连接状态和共享内存版本

---

## 🔍 数据说明

### 价差计算

1. **Diff1 = Bid_B - Ask_A**：
   - 正值：可以在A端做多，B端做空
   - 负值：无套利机会

2. **Diff2 = Bid_A - Ask_B**：
   - 正值：可以在B端做多，A端做空
   - 负值：无套利机会

3. **Max Spread**：
   - 取`max(Diff1, Diff2)`
   - 表示当前最佳套利机会

### 数据有效性

- **时间戳检查**：数据超过5秒未更新视为过期
- **有效性标志**：`isValid`必须为`true`
- **数据范围检查**：Bid和Ask必须大于0，且Ask > Bid

---

## ⚠️ 注意事项

1. **共享内存名称**：
   - GUI使用`HedgeArbEA_SharedMemory_{Symbol}`连接
   - 确保EA的`InpSymbol`与GUI的symbol一致

2. **数据更新**：
   - GUI每100ms更新一次显示
   - 如果EA停止更新数据，GUI会显示过期数据或"N/A"

3. **窗口大小**：
   - 可以调整窗口大小
   - 列宽会自动调整以适应窗口

---

## 📊 显示示例

### 正常显示（两个EA都在运行）：

```
Broker    | Bid       | Ask       | Spread   | Last Update
----------|-----------|-----------|----------|------------
Broker A  | 2650.50000| 2651.00000| 0.50000  | 50 ms ago
Broker B  | 2650.60000| 2651.10000| 0.50000  | 45 ms ago
Spread(B-A)| 0.10000  | -0.10000  | 0.10000  | 45 ms ago
Summary   | 2650.55000| 2651.05000| 0.50000  | Live
```

### EA未启动或数据过期：

```
Broker    | Bid  | Ask  | Spread | Last Update
----------|------|------|--------|------------
Broker A  | N/A  | N/A  | N/A    | N/A
Broker B  | N/A  | N/A  | N/A    | N/A
```

---

## 🔄 下一步优化（可选）

如果需要，可以添加：

1. **颜色标识**：
   - 价差大于阈值时高亮显示
   - 数据过期时红色显示

2. **历史图表**：
   - 显示价差变化趋势
   - 显示报价历史

3. **告警功能**：
   - 价差达到开仓条件时提示
   - 数据过期时告警

---

## ✅ 完成状态

- [x] 显示Broker A的Bid、Ask、Spread
- [x] 显示Broker B的Bid、Ask、Spread
- [x] 显示价差（Diff1和Diff2）
- [x] 显示汇总数据（平均值）
- [x] 显示数据更新时间
- [x] 自动更新（每100ms）
- [x] 数据有效性检查
- [x] 窗口大小自适应

**GUI报价显示功能已完成！**


