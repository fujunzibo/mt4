# Visual Studio 编译详细步骤

## 📋 准备工作

1. **确认源文件位置**：
   - 确保以下文件在 `D:\mt4\` 目录下：
     - `HedgeArbSharedMemory.h`
     - `HedgeArbSharedMemory.cpp`
     - `HedgeArbControlProgram.cpp`
     - `HedgeArbSharedMemory.def`（可选，用于导出函数）

2. **打开Visual Studio**（不需要打开MT4文件夹）

---

## 🔧 步骤1：创建DLL项目

### 1.1 创建新项目

**方法A：使用快捷键（推荐）**
1. 打开Visual Studio
2. 按 `Ctrl+Shift+N`（新建项目）

**方法B：使用起始页**
1. 打开Visual Studio
2. 在起始页点击"创建新项目"或"New Project"

**方法C：使用菜单**
1. 文件 → 新建 → 项目（或 文件 → 新建 → 项目...）

**然后：**
3. 在搜索框输入 "DLL" 或选择：
   - **Visual C++** → **Windows桌面** → **动态链接库(DLL)**
4. 点击"下一步"

**如果找不到"新建项目"：**
- 查看 → 菜单栏（确保菜单栏显示）
- 或按 `Alt` 键显示菜单栏
- 或查看工具栏是否有"新建项目"按钮

### 1.2 配置项目

1. **项目名称**：`HedgeArbSharedMemory`
2. **位置**：选择 `D:\mt4\`（或你存放源文件的目录）
   - ✅ **重要**：位置应该指向包含源文件的目录
   - Visual Studio会自动创建子文件夹 `HedgeArbSharedMemory`
3. **解决方案名称**：可以保持默认或改为 `HedgeArbEA`
4. 点击"创建"

### 1.3 删除自动生成的文件

1. 在解决方案资源管理器中，删除以下文件（如果存在）：
   - `pch.h`
   - `pch.cpp`
   - `dllmain.cpp`
   - `framework.h`
   - 右键 → 删除

### 1.4 添加源文件

1. 右键项目 `HedgeArbSharedMemory` → 添加 → 现有项
2. 选择以下文件（在 `D:\mt4\` 目录下）：
   - `HedgeArbSharedMemory.h`
   - `HedgeArbSharedMemory.cpp`
3. 点击"添加"

### 1.5 配置项目属性（重要！）

1. 右键项目 `HedgeArbSharedMemory` → 属性（或按 `Alt+Enter`）

2. **配置**：选择 `Release`（或 `Debug` 用于调试）

3. **平台**：选择 **Win32**（重要！MT4是32位程序）
   - 如果没有Win32选项：
     - 配置管理器 → 活动解决方案平台 → 新建
     - 选择 `Win32`，复制设置自 `x86`
     - 确定

4. **C/C++** → **常规**：
   - 附加包含目录：确保包含当前目录（通常是 `$(ProjectDir)`）

5. **链接器** → **输入**（可选，如果使用.def文件）：
   - 模块定义文件：`HedgeArbSharedMemory.def`

6. 点击"确定"保存

### 1.6 编译DLL

1. 生成 → 生成解决方案（或按 `F7`）
2. 查看输出窗口，应该显示：
   ```
   1>------ 已启动生成: 项目: HedgeArbSharedMemory, 配置: Release Win32 ------
   1>HedgeArbSharedMemory.cpp
   1>正在创建库 D:\mt4\HedgeArbSharedMemory\Release\HedgeArbSharedMemory.lib 和对象...
   1>正在创建 D:\mt4\HedgeArbSharedMemory\Release\HedgeArbSharedMemory.dll
   1>生成成功。
   ```

3. **输出文件位置**：
   - DLL：`D:\mt4\HedgeArbSharedMemory\Release\HedgeArbSharedMemory.dll`
   - LIB：`D:\mt4\HedgeArbSharedMemory\Release\HedgeArbSharedMemory.lib`

---

## 🔧 步骤2：创建总控程序项目

### 2.1 添加新项目到解决方案

**方法A：右键解决方案**
1. 右键解决方案 → 添加 → 新建项目
2. 选择：
   - **Visual C++** → **Windows桌面** → **控制台应用程序**
3. 点击"下一步"

**方法B：使用菜单**
1. 项目 → 添加新项目
2. 选择项目类型

**方法C：使用快捷键**
1. 按 `Ctrl+Shift+N` 创建新项目
2. 在"解决方案"下拉框中选择"添加到解决方案"

### 2.2 配置项目

1. **项目名称**：`HedgeArbControlProgram`
2. **位置**：保持默认（会在解决方案目录下创建）
3. 点击"创建"

### 2.3 删除自动生成的文件

1. 删除 `main.cpp`（如果存在）

### 2.4 添加源文件

1. 右键项目 `HedgeArbControlProgram` → 添加 → 现有项
2. 选择：
   - `HedgeArbControlProgram.cpp`（在 `D:\mt4\` 目录下）
   - `HedgeArbSharedMemory.h`（在 `D:\mt4\` 目录下，用于数据结构定义）
3. 点击"添加"

### 2.5 配置项目属性

1. 右键项目 `HedgeArbControlProgram` → 属性

2. **配置**：选择 `Release`
3. **平台**：选择 **Win32**

4. **C/C++** → **常规**：
   - 附加包含目录：添加 `$(SolutionDir)HedgeArbSharedMemory`（用于找到头文件）

5. **链接器** → **常规**：
   - 附加库目录：添加 `$(SolutionDir)HedgeArbSharedMemory\$(Platform)\$(Configuration)`
     - 这会解析为：`D:\mt4\HedgeArbSharedMemory\Release\`

6. **链接器** → **输入**：
   - 附加依赖项：添加 `HedgeArbSharedMemory.lib`

7. 点击"确定"保存

### 2.6 设置项目依赖

1. 右键解决方案 → 项目依赖项
2. 在"依赖于"中，勾选 `HedgeArbSharedMemory`
3. 点击"确定"

这确保在编译总控程序前，先编译DLL。

### 2.7 编译总控程序

1. 生成 → 生成解决方案（或按 `F7`）
2. 查看输出窗口，应该显示：
   ```
   1>------ 已启动生成: 项目: HedgeArbSharedMemory, 配置: Release Win32 ------
   1>生成成功。
   2>------ 已启动生成: 项目: HedgeArbControlProgram, 配置: Release Win32 ------
   2>HedgeArbControlProgram.cpp
   2>正在链接...
   2>正在创建 D:\mt4\HedgeArbControlProgram\Release\HedgeArbControlProgram.exe
   2>生成成功。
   ```

3. **输出文件位置**：
   - EXE：`D:\mt4\HedgeArbControlProgram\Release\HedgeArbControlProgram.exe`

---

## 📦 部署文件

### 1. 复制DLL到MT4

1. 找到编译好的DLL：
   - `D:\mt4\HedgeArbSharedMemory\Release\HedgeArbSharedMemory.dll`
2. 复制到MT4的Libraries目录：
   - 通常是：`C:\Program Files\MetaTrader 4\MQL4\Libraries\`
   - 或：`C:\Users\你的用户名\AppData\Roaming\MetaQuotes\Terminal\你的MT4实例\MQL4\Libraries\`

### 2. 复制总控程序

1. 找到编译好的EXE：
   - `D:\mt4\HedgeArbControlProgram\Release\HedgeArbControlProgram.exe`
2. 复制到合适的位置（如 `D:\mt4\`）

### 3. 配置总控程序

1. 编辑 `HedgeArbControlProgram.cpp`，修改配置：
   ```cpp
   const char* SYMBOL_NAME = "GOLD";  // 改为你使用的品种
   const char* DATA_DIR = "D:\\mt4\\test\\";  // 改为你的数据目录
   ```
2. 重新编译总控程序

---

## ✅ 验证编译成功

### 检查文件

1. **DLL文件**：
   - 位置：MT4的 `MQL4\Libraries\HedgeArbSharedMemory.dll`
   - 大小：应该有几KB到几十KB
   - 右键 → 属性 → 详细信息，应该显示"32位"

2. **EXE文件**：
   - 位置：`D:\mt4\HedgeArbControlProgram.exe`（或你复制的位置）
   - 可以双击运行测试

### 测试DLL

1. 在MT4中：
   - 工具 → 选项 → 专家顾问
   - 勾选"允许DLL导入"
2. 加载EA，查看日志：
   - 如果看到"共享内存连接初始化成功"，说明DLL工作正常
   - 如果看到"无法初始化共享内存连接"，检查总控程序是否运行

---

## 🐛 常见问题

### Q: 编译时提示找不到头文件

**A:** 检查：
1. 附加包含目录是否正确设置
2. 头文件是否在正确位置

### Q: 链接时提示找不到.lib文件

**A:** 检查：
1. DLL项目是否先编译成功
2. 附加库目录是否正确
3. 附加依赖项中是否添加了 `HedgeArbSharedMemory.lib`

### Q: 编译为x64而不是Win32

**A:** 
1. 配置管理器 → 活动解决方案平台 → 选择 Win32
2. 如果列表中没有Win32，点击"新建"创建

### Q: MT4提示无法加载DLL

**A:** 检查：
1. DLL是否在 `MQL4\Libraries\` 目录
2. MT4是否允许DLL导入（工具 → 选项 → 专家顾问）
3. DLL是否为32位（右键DLL → 属性 → 详细信息）

---

## 📝 总结

- ✅ **不需要打开MT4文件夹**
- ✅ **在Visual Studio中创建新项目**
- ✅ **项目位置指向包含源文件的目录（如 `D:\mt4\`）**
- ✅ **必须编译为Win32平台（32位）**
- ✅ **先编译DLL，再编译总控程序**

