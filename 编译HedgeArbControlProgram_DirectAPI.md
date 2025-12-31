# 编译 HedgeArbControlProgram_DirectAPI 指南

## 问题：cl 命令无法识别

在普通 PowerShell 中，`cl` 命令无法识别，因为 Visual Studio 的编译环境没有加载。

## 解决方法

### 方法1：使用 Visual Studio 开发者命令提示符（推荐）

1. **打开开发者命令提示符**
   - 开始菜单 → 搜索 "Developer Command Prompt for VS"
   - 或：开始菜单 → Visual Studio 文件夹 → Developer Command Prompt for VS

2. **切换到项目目录**
   ```cmd
   cd D:\mt4
   ```

3. **编译**
   ```cmd
   cl /EHsc /std:c++11 /I. HedgeArbControlProgram_DirectAPI.cpp ForexAPI\ForexComAPI.cpp ForexAPI\HttpClient.cpp /link comctl32.lib /SUBSYSTEM:WINDOWS /OUT:HedgeArbControlProgram_DirectAPI.exe
   ```

### 方法2：在 PowerShell 中加载 Visual Studio 环境

在 PowerShell 中运行：

```powershell
# 找到 Visual Studio 安装路径（通常是以下之一）
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
# 或者
$vsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"

# 加载环境（注意：PowerShell 中需要使用 cmd 来运行 .bat 文件）
cmd /c "`"$vsPath`" x86 && cd D:\mt4 && cl /EHsc /std:c++11 /I. HedgeArbControlProgram_DirectAPI.cpp ForexAPI\ForexComAPI.cpp ForexAPI\HttpClient.cpp /link comctl32.lib /SUBSYSTEM:WINDOWS /OUT:HedgeArbControlProgram_DirectAPI.exe"
```

### 方法3：在 Visual Studio IDE 中编译（最简单）

这是最推荐的方法：

1. **打开 Visual Studio**

2. **创建新项目**
   - 按 `Ctrl+Shift+N` 或 文件 → 新建 → 项目
   - 选择：**Visual C++** → **Windows桌面** → **Windows桌面应用程序**
   - 项目名称：`HedgeArbControlProgramDirectAPI`
   - 位置：`D:\mt4\`
   - 点击"创建"

3. **删除自动生成的文件**
   - 在解决方案资源管理器中，删除自动生成的 `.cpp` 和 `.h` 文件

4. **添加源文件**
   - 右键项目 → 添加 → 现有项
   - 选择以下文件：
     - `HedgeArbControlProgram_DirectAPI.cpp`
     - `ForexAPI\ForexComAPI.cpp`
     - `ForexAPI\HttpClient.cpp`
     - `ForexAPI\ForexComAPI.h`
     - `ForexAPI\HttpClient.h`

5. **配置项目属性**
   - 右键项目 → 属性
   - **配置**：Release
   - **平台**：Win32（或 x64）
   
   - **C/C++** → **常规**：
     - 附加包含目录：添加 `$(ProjectDir)..\` 或 `D:\mt4\`
     - C++语言标准：ISO C++11 或更高
   
   - **链接器** → **系统**：
     - 子系统：Windows (/SUBSYSTEM:WINDOWS)
   
   - **链接器** → **输入**：
     - 附加依赖项：`comctl32.lib`

6. **编译**
   - 生成 → 生成解决方案（F7）
   - 或按 `Ctrl+Shift+B`

7. **运行**
   - 按 `F5` 运行
   - 或：调试 → 开始调试

### 方法4：添加到现有解决方案

如果您已经有 `mt4.sln`：

1. **打开解决方案**
   - 打开 `D:\mt4\mt4.sln`

2. **添加新项目**
   - 右键解决方案 → 添加 → 新建项目
   - 选择：**Visual C++** → **Windows桌面** → **Windows桌面应用程序**
   - 项目名称：`HedgeArbControlProgramDirectAPI`
   - 点击"创建"

3. **后续步骤同方法3的步骤3-6**

## 快速检查清单

编译前确保：
- ✅ Visual Studio 已安装
- ✅ 已安装"使用C++的桌面开发"工作负载
- ✅ 所有源文件都在正确位置
- ✅ 包含目录配置正确

## 编译输出

编译成功后，会生成：
- `HedgeArbControlProgramDirectAPI.exe`（可执行文件）
- `HedgeArbControlProgramDirectAPI.pdb`（调试符号文件，Debug模式）

## 运行程序

直接双击 `HedgeArbControlProgramDirectAPI.exe` 运行，或在 Visual Studio 中按 `F5` 运行。

