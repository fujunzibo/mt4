# 直接API主控程序快速编译指南

## ✅ 是的，可以直接编译运行！

`HedgeArbControlProgram_DirectAPI.cpp` 是一个独立的程序，可以直接编译运行。

## 方法1：使用 Visual Studio 命令行编译（最快）

### 步骤：

1. **打开"开发者命令提示符"**
   - 在开始菜单搜索 "Developer Command Prompt for VS"
   - 或者：开始菜单 → Visual Studio → 开发者命令提示符

2. **切换到项目目录**
   ```cmd
   cd D:\mt4
   ```

3. **编译程序**
   ```cmd
   cl /EHsc /std:c++11 /I. HedgeArbControlProgram_DirectAPI.cpp ForexAPI\ForexComAPI.cpp ForexAPI\HttpClient.cpp /link comctl32.lib /SUBSYSTEM:WINDOWS /OUT:HedgeArbControlProgram_DirectAPI.exe
   ```

4. **运行**
   ```cmd
   HedgeArbControlProgram_DirectAPI.exe
   ```

## 方法2：在 Visual Studio 中创建简单项目

### 步骤：

1. **打开 Visual Studio**

2. **创建新项目**
   - 按 `Ctrl+Shift+N` 或 文件 → 新建 → 项目
   - 选择：**Visual C++** → **Windows桌面** → **Windows桌面应用程序**
   - 项目名称：`HedgeArbControlProgramDirectAPI`
   - 位置：`D:\mt4\`（或任意位置）
   - 点击"创建"

3. **删除自动生成的文件**
   - 删除 `HedgeArbControlProgramDirectAPI.cpp`（如果存在）
   - 删除 `HedgeArbControlProgramDirectAPI.h`（如果存在）
   - 删除 `framework.h`、`targetver.h` 等

4. **添加源文件**
   - 右键项目 → 添加 → 现有项
   - 选择以下文件：
     - `HedgeArbControlProgram_DirectAPI.cpp`（根目录）
     - `ForexAPI\ForexComAPI.cpp`
     - `ForexAPI\HttpClient.cpp`
     - `ForexAPI\ForexComAPI.h`
     - `ForexAPI\HttpClient.h`

5. **配置项目属性**
   - 右键项目 → 属性
   - **配置**：Release
   - **平台**：Win32（或 x64，根据您的系统）
   
   - **C/C++** → **常规**：
     - 附加包含目录：添加 `$(ProjectDir)..\`（或 `D:\mt4\`）
     - C++语言标准：ISO C++11 或更高
   
   - **链接器** → **系统**：
     - 子系统：Windows (/SUBSYSTEM:WINDOWS)
   
   - **链接器** → **输入**：
     - 附加依赖项：`comctl32.lib`

6. **编译**
   - 生成 → 生成解决方案（F7）
   - 输出：`Release\HedgeArbControlProgramDirectAPI.exe`

## 方法3：添加到现有解决方案

如果您已经有 `mt4.sln` 解决方案：

1. **打开解决方案**
   - 打开 `mt4.sln`

2. **添加新项目**
   - 右键解决方案 → 添加 → 新建项目
   - 选择：**Visual C++** → **Windows桌面** → **Windows桌面应用程序**
   - 项目名称：`HedgeArbControlProgramDirectAPI`
   - 点击"创建"

3. **后续步骤同方法2的步骤3-6**

## 编译依赖

### 必需文件：
- ✅ `HedgeArbControlProgram_DirectAPI.cpp`（主程序）
- ✅ `ForexAPI\ForexComAPI.cpp`（API客户端实现）
- ✅ `ForexAPI\ForexComAPI.h`（API客户端头文件）
- ✅ `ForexAPI\HttpClient.cpp`（HTTP客户端实现）
- ✅ `ForexAPI\HttpClient.h`（HTTP客户端头文件）

### 系统库：
- ✅ `comctl32.lib`（Windows Common Controls）
- ✅ `winhttp.lib`（Windows HTTP，已包含在Windows SDK中）

## 编译选项说明

### 命令行参数：
- `/EHsc`：启用C++异常处理
- `/std:c++11`：使用C++11标准（支持std::thread等）
- `/I.`：添加当前目录到包含路径
- `/SUBSYSTEM:WINDOWS`：Windows GUI程序（不是控制台程序）
- `/link comctl32.lib`：链接Common Controls库

### 项目属性：
- **字符集**：可以使用"使用多字节字符集"或"使用Unicode字符集"
- **运行时库**：可以使用 `/MD`（动态链接）或 `/MT`（静态链接）

## 常见问题

### 问题1：找不到头文件

**错误**：`fatal error C1083: 无法打开包含文件: "../ForexAPI/ForexComAPI.h"`

**解决方法**：
- 检查文件路径是否正确
- 在项目属性中添加包含目录：`$(ProjectDir)..\` 或 `D:\mt4\`

### 问题2：链接错误

**错误**：`unresolved external symbol`

**解决方法**：
- 确保添加了所有必需的源文件（.cpp文件）
- 确保链接了 `comctl32.lib`

### 问题3：C++11特性不支持

**错误**：`'thread' is not a member of 'std'`

**解决方法**：
- 设置C++标准为C++11或更高
- 项目属性 → C/C++ → 语言 → C++语言标准

## 运行程序

编译成功后，直接运行 `HedgeArbControlProgramDirectAPI.exe`：

1. **首次运行**：
   - 程序会显示GUI界面
   - 需要配置API Key和Secret
   - 点击"连接API"连接服务器

2. **使用**：
   - 在界面中设置所有参数
   - 点击"启动交易"开始自动交易

## 优势

✅ **独立运行**：不需要DLL文件  
✅ **无需MT4**：直接通过API交易  
✅ **快速编译**：单个命令即可编译  
✅ **易于部署**：只需一个exe文件（如果静态链接）

## 下一步

编译成功后，参考 `直接API交易主控程序使用说明.md` 了解如何使用程序。

