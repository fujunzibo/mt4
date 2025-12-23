# 编译说明（不使用CMake）

## 方法1：使用Visual Studio直接编译（推荐）

### 步骤1：创建DLL项目

1. 打开Visual Studio
2. 文件 → 新建 → 项目
3. 选择"Visual C++" → "Windows桌面" → "动态链接库(DLL)"
4. 项目名称：`HedgeArbSharedMemory`
5. 位置：选择项目目录（如 `D:\mt4\`）

### 步骤2：添加源文件

1. 删除自动生成的 `dllmain.cpp`、`pch.cpp`、`pch.h`（如果存在）
2. 右键项目 → 添加 → 现有项
3. 添加以下文件：
   - `HedgeArbSharedMemory.h`
   - `HedgeArbSharedMemory.cpp`

### 步骤3：配置项目属性

1. 右键项目 → 属性
2. **常规**：
   - 平台工具集：选择你的Visual Studio版本
   - Windows SDK版本：选择已安装的版本
3. **C/C++** → **常规**：
   - 附加包含目录：确保包含当前目录
4. **链接器** → **常规**：
   - 输出文件：`$(OutDir)$(TargetName)$(TargetExt)`
5. **配置类型**：确保是"动态库(.dll)"
6. **平台**：选择 **Win32**（重要！MT4是32位程序）

### 步骤4：编译DLL

1. 生成 → 生成解决方案（或按 F7）
2. 输出文件在：`项目目录\x86\Release\HedgeArbSharedMemory.dll` 或 `项目目录\Win32\Release\HedgeArbSharedMemory.dll`

---

### 步骤5：创建总控程序项目

1. 文件 → 新建 → 项目
2. 选择"Visual C++" → "Windows桌面" → "控制台应用程序"
3. 项目名称：`HedgeArbControlProgram`
4. 位置：选择项目目录（如 `D:\mt4\`）

### 步骤6：添加源文件

1. 删除自动生成的 `main.cpp`（如果存在）
2. 右键项目 → 添加 → 现有项
3. 添加以下文件：
   - `HedgeArbControlProgram.cpp`
   - `HedgeArbSharedMemory.h`（仅头文件，用于数据结构定义）

### 步骤7：配置项目属性

1. 右键项目 → 属性
2. **C/C++** → **常规**：
   - 附加包含目录：确保包含当前目录
3. **链接器** → **输入**：
   - 附加依赖项：添加 `HedgeArbSharedMemory.lib`（需要先编译DLL生成）
4. **链接器** → **常规**：
   - 附加库目录：添加DLL项目的输出目录（如 `$(SolutionDir)HedgeArbSharedMemory\$(Platform)\$(Configuration)\`）
5. **平台**：选择 **Win32**

### 步骤8：设置项目依赖

1. 右键解决方案 → 项目依赖项
2. 选择 `HedgeArbControlProgram` 依赖于 `HedgeArbSharedMemory`

### 步骤9：编译总控程序

1. 生成 → 生成解决方案（或按 F7）
2. 输出文件在：`项目目录\x86\Release\HedgeArbControlProgram.exe` 或 `项目目录\Win32\Release\HedgeArbControlProgram.exe`

---

## 方法2：使用命令行编译（需要Visual Studio）

### 步骤1：打开"Developer Command Prompt for VS"

1. 开始菜单 → Visual Studio → "Developer Command Prompt for VS 2019"（或对应版本）
2. 进入项目目录：
   ```cmd
   cd D:\mt4
   ```

### 步骤2：编译DLL

```cmd
cl /LD /MT /O2 /W3 /D "WIN32" /D "_WINDOWS" /D "_USRDLL" HedgeArbSharedMemory.cpp /Fe:HedgeArbSharedMemory.dll /link /DEF:HedgeArbSharedMemory.def
```

如果没有`.def`文件，可以创建：

**HedgeArbSharedMemory.def**:
```
EXPORTS
InitSharedMemory
SetQuote
GetQuote
SetProfit
GetProfit
SetCommand
GetCommand
ClearCommand
SetAck
GetAck
ClearAck
CloseSharedMemory
```

### 步骤3：编译总控程序

```cmd
cl /MT /O2 /W3 /D "WIN32" HedgeArbControlProgram.cpp HedgeArbSharedMemory.lib /Fe:HedgeArbControlProgram.exe
```

---

## 方法3：安装CMake（如果仍想使用CMake）

### Windows安装CMake

1. 下载CMake：
   - 访问：https://cmake.org/download/
   - 下载Windows安装程序（.msi文件）

2. 安装：
   - 运行安装程序
   - **重要**：勾选"Add CMake to system PATH"

3. 验证安装：
   ```cmd
   cmake --version
   ```

4. 如果仍无法识别，手动添加到PATH：
   - 右键"此电脑" → 属性 → 高级系统设置 → 环境变量
   - 在"系统变量"的Path中添加CMake的bin目录（如 `C:\Program Files\CMake\bin`）

---

## 重要提示

1. **必须编译为32位（Win32）**：MT4是32位程序，DLL必须是32位
2. **Release配置**：生产环境使用Release配置以获得最佳性能
3. **DLL导出**：确保所有函数都正确导出（使用`__declspec(dllexport)`）

---

## 快速检查清单

- [ ] DLL编译为Win32平台
- [ ] DLL输出文件存在
- [ ] 总控程序编译成功
- [ ] 所有文件在正确位置


