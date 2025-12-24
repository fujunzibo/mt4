# DLL和EX4文件说明

## 📋 重要说明

**DLL文件和EX4文件都需要，它们配合工作！**

---

## 🔍 两者的关系

### 1. **HedgeArbEA.ex4**（MQL4编译后的EA文件）

**作用**：
- EA的主程序
- 包含所有交易逻辑（开仓、平仓、止损等）
- 包含与MT4交互的代码（读取报价、下单等）

**位置**：
- 编译后生成：`MT4安装目录\MQL4\Experts\HedgeArbEA.ex4`
- 或者：`C:\Users\你的用户名\AppData\Roaming\MetaQuotes\Terminal\[实例]\MQL4\Experts\HedgeArbEA.ex4`

**如何生成**：
- 在MetaEditor中打开 `HedgeArbEA.mq4`
- 按 `F7` 编译
- 编译成功后自动生成 `.ex4` 文件

---

### 2. **HedgeArbSharedMemory.dll**（C++编译的DLL文件）

**作用**：
- 提供共享内存通信功能
- 供EA调用，实现两个MT4实例之间的数据交换
- 不包含交易逻辑，只是通信工具

**位置**：
- 编译后生成：`D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.dll`
- 部署到：`MT4安装目录\MQL4\Libraries\HedgeArbSharedMemory.dll`

**如何生成**：
- 在Visual Studio中编译 `HedgeArbSharedMemory` 项目
- 编译成功后生成 `.dll` 文件

---

## 🔗 它们如何配合工作

### 工作流程：

```
┌─────────────────┐
│  HedgeArbEA.ex4 │  ← EA主程序（交易逻辑）
│   (MT4 A端)     │
└────────┬────────┘
         │ 调用DLL函数
         ▼
┌─────────────────────────┐
│ HedgeArbSharedMemory.dll │  ← 共享内存通信库
│   (在Libraries目录)      │
└────────┬─────────────────┘
         │ 访问共享内存
         ▼
┌─────────────────┐
│   共享内存区域   │  ← 数据交换中心
└────────┬────────┘
         │
         ▼
┌─────────────────────────┐
│ HedgeArbSharedMemory.dll │
└────────┬─────────────────┘
         │ 调用DLL函数
         ▼
┌─────────────────┐
│  HedgeArbEA.ex4 │  ← EA主程序（交易逻辑）
│   (MT4 B端)     │
└─────────────────┘
```

### 代码示例：

在 `HedgeArbEA.mq4` 中：

```mql4
//--- DLL导入（共享内存通信）
#import "HedgeArbSharedMemory.dll"
   bool InitSharedMemory(string symbol);
   bool SetQuote(string brokerId, double bid, double ask);
   bool GetQuote(string brokerId, double &bid, double &ask);
   // ... 其他函数
#import

// EA的主逻辑
void OnTick() {
    // 1. EA自己的逻辑（读取MT4报价、下单等）
    double bid = MarketInfo(Symbol(), MODE_BID);
    double ask = MarketInfo(Symbol(), MODE_ASK);
    
    // 2. 调用DLL函数，将报价写入共享内存
    SetQuote("A", bid, ask);
    
    // 3. 调用DLL函数，从共享内存读取对方报价
    double bidB, askB;
    GetQuote("B", bidB, askB);
    
    // 4. EA自己的逻辑（计算价差、决定是否开仓等）
    // ...
}
```

---

## ✅ 完整的部署清单

### 必须部署的文件：

1. **HedgeArbEA.ex4**（EA主程序）
   - 位置：`MQL4\Experts\HedgeArbEA.ex4`
   - 来源：在MetaEditor中编译 `HedgeArbEA.mq4` 生成

2. **HedgeArbSharedMemory.dll**（共享内存库）
   - 位置：`MQL4\Libraries\HedgeArbSharedMemory.dll`
   - 来源：Visual Studio编译生成

3. **HedgeArbControlProgramGUI.exe**（GUI总控程序，可选但推荐）
   - 位置：任意位置（如 `D:\mt4\`）
   - 来源：Visual Studio编译生成

---

## 🔧 编译和部署步骤

### 步骤1：编译MQL4 EA

1. **打开MetaEditor**
   - 在MT4中按 `F4`
   - 或：工具 → MetaQuotes语言编辑器

2. **打开EA文件**
   - 在导航器中找到 `HedgeArbEA.mq4`
   - 双击打开

3. **编译**
   - 按 `F7` 键
   - 或点击工具栏的"编译"按钮

4. **检查编译结果**
   - 底部"工具"窗口应显示：`0 error(s), 0 warning(s)`
   - 如果成功，`.ex4` 文件会自动生成在 `Experts` 目录

5. **验证文件**
   - 检查 `MQL4\Experts\` 目录下是否有 `HedgeArbEA.ex4`

---

### 步骤2：编译C++ DLL

1. **在Visual Studio中编译**
   - 打开 `HedgeArbEA.slnx` 解决方案
   - 选择 **Release Win32** 配置
   - 生成 → 生成解决方案（`F7`）

2. **找到编译输出**
   - DLL文件：`D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.dll`

3. **部署DLL**
   - 复制DLL到MT4的 `MQL4\Libraries\` 目录

---

### 步骤3：启用DLL导入

1. **在MT4中设置**
   - 工具 → 选项 → 专家顾问
   - 勾选"允许DLL导入"
   - 点击"确定"

2. **重启MT4**（如果已打开）

---

## 🎯 为什么两者都需要？

### 如果只有DLL，没有EX4：
- ❌ MT4无法运行EA
- ❌ 没有交易逻辑
- ❌ 无法下单、平仓

### 如果只有EX4，没有DLL：
- ❌ EA无法调用共享内存函数
- ❌ 两个MT4实例无法通信
- ❌ 编译时会报错：找不到DLL函数

### 两者都有：
- ✅ EA可以正常运行
- ✅ 可以调用DLL函数
- ✅ 两个MT4实例可以通信
- ✅ 完整的套利功能

---

## 📝 文件对应关系

| 文件类型 | 源文件 | 编译后文件 | 部署位置 | 作用 |
|---------|--------|-----------|---------|------|
| **EA主程序** | `HedgeArbEA.mq4` | `HedgeArbEA.ex4` | `MQL4\Experts\` | 交易逻辑 |
| **共享内存库** | `HedgeArbSharedMemory.cpp` | `HedgeArbSharedMemory.dll` | `MQL4\Libraries\` | 通信功能 |
| **GUI程序** | `HedgeArbControlProgramGUI.cpp` | `HedgeArbControlProgramGUI.exe` | 任意位置 | 监控界面 |

---

## 🔍 验证部署

### 检查清单：

- [ ] `MQL4\Experts\HedgeArbEA.ex4` 存在
- [ ] `MQL4\Libraries\HedgeArbSharedMemory.dll` 存在
- [ ] MT4已启用"允许DLL导入"
- [ ] 在MT4中可以找到并加载EA
- [ ] EA加载后没有报错
- [ ] EA日志显示"共享内存连接初始化成功"

---

## ❓ 常见问题

### Q: 我只部署了DLL，为什么EA无法运行？

**A:** 因为缺少EA主程序（.ex4文件）。DLL只是工具库，EA主程序才是执行交易逻辑的程序。

### Q: 我只编译了.mq4文件，为什么EA报错找不到DLL函数？

**A:** 因为缺少DLL文件。EA代码中使用了 `#import "HedgeArbSharedMemory.dll"`，必须要有对应的DLL文件。

### Q: 修改了.mq4文件后，需要重新编译DLL吗？

**A:** 不需要。只要DLL的接口（函数签名）没有变化，就不需要重新编译DLL。只需要重新编译.mq4文件生成新的.ex4文件。

### Q: 修改了DLL代码后，需要重新编译.mq4吗？

**A:** 不需要。只要DLL的接口（函数签名）没有变化，就不需要重新编译.mq4。只需要重新编译DLL并替换文件。

### Q: 两个MT4实例需要各自的.ex4文件吗？

**A:** 不需要。同一个.ex4文件可以在两个MT4实例中使用，只需要通过参数（`InpBrokerId`）区分即可。

### Q: 两个MT4实例需要各自的.dll文件吗？

**A:** 不需要。同一个.dll文件可以供两个MT4实例使用，它们通过共享内存通信。

---

## 📌 总结

**简单记忆**：
- **.ex4文件** = EA的主程序（必须）
- **.dll文件** = EA使用的工具库（必须）
- **两者配合** = 完整的套利系统

**部署顺序**：
1. 编译.mq4 → 生成.ex4
2. 编译DLL → 生成.dll
3. 部署.ex4到Experts目录
4. 部署.dll到Libraries目录
5. 启用DLL导入
6. 加载EA运行



