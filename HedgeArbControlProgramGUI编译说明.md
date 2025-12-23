# HedgeArbControlProgramGUI 编译说明

## 概述

`HedgeArbControlProgramGUI.cpp` 是一个基于 Win32 API 的 Windows 窗体程序，集成了总控程序的所有功能，并提供了图形界面显示：
- 实时报价
- 指令状态
- 盈亏信息
- 历史记录

## 编译步骤

### 方法1：在现有解决方案中添加

1. **打开 Visual Studio**
   - 打开现有的 `mt4.sln` 解决方案

2. **添加新项目**
   - 右键解决方案 → 添加 → 新建项目
   - 选择：**Visual C++** → **Windows桌面** → **Windows桌面应用程序**
   - 项目名称：`HedgeArbControlProgramGUI`
   - 点击"创建"

3. **删除自动生成的文件**
   - 删除 `HedgeArbControlProgramGUI.cpp`（如果存在）
   - 删除 `HedgeArbControlProgramGUI.h`（如果存在）
   - 删除 `framework.h`、`targetver.h` 等自动生成的文件

4. **添加源文件**
   - 右键项目 → 添加 → 现有项
   - 选择 `HedgeArbControlProgramGUI.cpp`（在项目根目录）
   - 选择 `HedgeArbSharedMemory.h`（在项目根目录）

5. **配置项目属性**
   - 右键项目 → 属性
   - **配置**：选择 `Release`
   - **平台**：选择 **Win32**（重要！MT4是32位程序）

   - **C/C++** → **常规**：
     - 附加包含目录：添加 `$(SolutionDir)`（用于找到头文件）

   - **链接器** → **常规**：
     - 附加库目录：添加 `$(SolutionDir)HedgeArbSharedMemory\$(Platform)\$(Configuration)`

   - **链接器** → **输入**：
     - 附加依赖项：添加 `comctl32.lib`（用于ListView和StatusBar控件）

6. **设置项目依赖**
   - 右键解决方案 → 项目依赖项
   - 在"依赖于"中，勾选 `HedgeArbSharedMemory`

7. **编译**
   - 生成 → 生成解决方案（或按 `F7`）

### 方法2：创建新的控制台项目并修改

1. **创建控制台项目**
   - 按照《Visual Studio编译详细步骤.md》中的步骤2创建控制台项目

2. **修改项目属性**
   - 右键项目 → 属性
   - **链接器** → **系统**：
     - 子系统：改为 **Windows (/SUBSYSTEM:WINDOWS)**
   - **链接器** → **输入**：
     - 附加依赖项：添加 `comctl32.lib`

3. **替换源文件**
   - 将 `HedgeArbControlProgram.cpp` 的内容替换为 `HedgeArbControlProgramGUI.cpp` 的内容
   - 或者直接重命名文件

4. **编译**
   - 生成 → 生成解决方案

## 配置修改

在 `HedgeArbControlProgramGUI.cpp` 中修改以下配置：

```cpp
// 配置
const char* SYMBOL_NAME = "GOLD";  // 根据实际使用的品种修改
const char* DATA_DIR = "D:\\mt4\\test\\";  // 数据文件目录
const int UPDATE_INTERVAL_MS = 100;  // 界面更新间隔（毫秒）
```

## 运行

1. **确保DLL已部署**
   - `HedgeArbSharedMemory.dll` 必须在MT4的 `MQL4\Libraries\` 目录

2. **运行程序**
   - 双击编译生成的 `HedgeArbControlProgramGUI.exe`
   - 或从Visual Studio按 `F5` 运行

3. **程序功能**
   - 自动连接共享内存
   - 每100毫秒更新一次界面显示
   - 实时显示报价、指令状态、盈亏信息
   - 自动读取并显示历史记录
   - 检测平仓完成并写入历史记录文件

## 界面说明

### 实时报价
- 显示 Broker A 和 Broker B 的 Bid、Ask、中间价
- 显示价差（A-B）

### 指令状态
- 显示当前活动指令（OPEN、CLOSEALL等）
- 显示做多端和做空端
- 显示两个Broker的确认状态

### 盈亏信息
- 显示 Broker A 和 Broker B 的盈亏（点和USD）
- 显示合计盈亏

### 历史记录
- 从CSV文件读取并显示最近20条交易记录
- 显示时间、Broker、订单号、类型、盈亏

### 状态栏
- 显示数据版本号和品种名称

## 注意事项

1. **必须编译为Win32平台**（32位），因为MT4是32位程序
2. **需要先编译DLL**：确保 `HedgeArbSharedMemory` 项目先编译成功
3. **数据目录**：确保 `DATA_DIR` 指向的目录存在
4. **共享内存**：程序会自动创建或打开共享内存，无需手动启动控制台版本

## 与C#版本的区别

- **性能更好**：直接访问共享内存，无跨语言调用开销
- **集成度高**：总控功能和界面显示完全融合
- **依赖更少**：不需要.NET运行时
- **体积更小**：编译后的exe文件更小

## 故障排除

### 问题：程序启动后显示"无法初始化共享内存"
**解决**：
- 检查MT4是否已加载EA
- 检查DLL是否在正确位置
- 检查品种名称是否匹配

### 问题：界面不更新
**解决**：
- 检查共享内存是否正常
- 检查MT4 EA是否在运行
- 查看是否有错误消息

### 问题：编译错误
**解决**：
- 确保包含目录正确设置
- 确保链接库目录正确
- 确保 `comctl32.lib` 已添加到依赖项


