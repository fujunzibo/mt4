# 在 VS Code 中使用命令行编译（备选方案）

## ⚠️ 前提条件

即使使用 VS Code，你仍然需要安装 Visual Studio 的编译工具。

---

## 📥 安装 Visual Studio Build Tools（最小安装）

如果你不想安装完整的 Visual Studio，可以只安装编译工具：

1. **下载 Build Tools**：
   - https://visualstudio.microsoft.com/zh-hans/downloads/
   - 滚动到底部，找到"所有下载"
   - 下载"Visual Studio 生成工具"

2. **安装时选择**：
   - ✅ **使用C++的桌面开发**

---

## 🔧 在 VS Code 中编译

### 方法1：使用 Developer Command Prompt

1. **打开 Developer Command Prompt**：
   - 开始菜单 → Visual Studio → "Developer Command Prompt for VS 2022"（或对应版本）

2. **进入项目目录**：
   ```cmd
   cd D:\mt4
   ```

3. **编译DLL**：
   ```cmd
   cl /LD /MT /O2 /W3 /D "WIN32" /D "_WINDOWS" /D "_USRDLL" HedgeArbSharedMemory.cpp /Fe:HedgeArbSharedMemory.dll /link /DEF:HedgeArbSharedMemory.def
   ```

4. **编译总控程序**：
   ```cmd
   cl /MT /O2 /W3 /D "WIN32" HedgeArbControlProgram.cpp HedgeArbSharedMemory.lib /Fe:HedgeArbControlProgram.exe
   ```

### 方法2：在 VS Code 中使用终端

1. **在 VS Code 中打开终端**：
   - 按 `Ctrl+` `（反引号）
   - 或：终端 → 新建终端

2. **切换到 Developer Command Prompt**：
   ```powershell
   & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
   ```
   （路径可能不同，根据你的安装位置调整）

3. **然后使用上面的编译命令**

---

## ⚠️ 推荐方案

**强烈推荐安装完整的 Visual Studio**，因为：
- ✅ 图形界面，更容易使用
- ✅ 自动配置项目
- ✅ 更好的错误提示
- ✅ 调试功能

命令行编译适合有经验的开发者。


