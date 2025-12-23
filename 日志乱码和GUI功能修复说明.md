# 日志乱码和GUI功能修复说明

## 🔍 问题分析

### 1. 日志乱码问题

**问题**：MT4日志中显示大量 `？？？？？？？？？？？` 乱码

**原因**：
- MQL4文件中的中文字符在MT4中可能因为编码问题无法正确显示
- MT4的日志系统可能不支持某些中文字符编码

**解决方案**：
- 将关键的中文日志改为英文，避免编码问题
- 已修复的日志：
  - `【发送指令】` → `[SendCommand] Command sent to shared memory:`
  - `【新开仓逻辑】发送开仓指令：` → `[SendOpenCommand] Command sent:`
  - `（已写入共享内存）` → `(sent to shared memory)`

### 2. GUI逻辑品种修改功能

**问题**：总控界面中的逻辑品种是硬编码的，无法修改

**解决方案**：
- 将 `SYMBOL_NAME` 从常量改为全局变量 `g_SymbolName`
- 添加了逻辑品种输入框（Edit控件）
- 添加了"应用"按钮
- 当用户修改逻辑品种并点击"应用"时：
  1. 关闭旧的共享内存连接
  2. 更新逻辑品种名称
  3. 重新初始化共享内存连接
  4. 如果成功，显示成功消息并更新显示
  5. 如果失败，显示错误消息并恢复原值

## ✅ 已完成的修改

### 1. HedgeArbEA.mq4

**修改的日志**：
```mql4
// 修改前
Print("【发送指令】", cmd, "（已写入共享内存）");
Print("【新开仓逻辑】发送开仓指令：", cmd, " 价差=", DoubleToString(useDiffPips, 2), " 点");

// 修改后
Print("[SendCommand] Command sent to shared memory: ", cmd);
Print("[SendOpenCommand] Command sent: ", cmd, " Spread=", DoubleToString(useDiffPips, 2), " pips");
```

### 2. HedgeArbControlProgramGUI.cpp

**添加的功能**：
1. **全局变量**：
   ```cpp
   char g_SymbolName[32] = "GOLD";  // 逻辑品种名称（可修改）
   HWND g_hEditSymbol = NULL;       // 逻辑品种输入框
   HWND g_hBtnApply = NULL;         // 应用按钮
   ```

2. **控件ID**：
   ```cpp
   #define ID_EDIT_SYMBOL    3001
   #define ID_BTN_APPLY      3002
   ```

3. **创建控件**（在 `WM_CREATE` 中）：
   - 标签："逻辑品种:"
   - 输入框：显示当前逻辑品种（默认"GOLD"）
   - 按钮："应用"

4. **按钮点击处理**（在 `WM_COMMAND` 中）：
   - 读取输入框的值
   - 去除首尾空格
   - 如果值有变化：
     - 关闭旧的共享内存连接
     - 更新逻辑品种名称
     - 重新初始化共享内存
     - 显示成功或错误消息

5. **关闭共享内存函数**：
   ```cpp
   void CloseSharedMemory() {
       if (g_pSharedData) {
           UnmapViewOfFile(g_pSharedData);
           g_pSharedData = NULL;
       }
       if (g_hMapFile) {
           CloseHandle(g_hMapFile);
           g_hMapFile = NULL;
       }
   }
   ```

6. **资源清理**（在 `WM_DESTROY` 中）：
   - 调用 `CloseSharedMemory()` 确保资源正确释放

## 📋 使用说明

### 修改逻辑品种

1. **启动总控程序**
2. **在"逻辑品种"输入框中输入新的品种名称**（例如：`XAUUSD`、`EURUSD`等）
3. **点击"应用"按钮**
4. **如果成功**：
   - 显示成功消息
   - 界面自动更新，连接到新的共享内存
5. **如果失败**：
   - 显示错误消息
   - 输入框恢复为原来的值
   - 请检查：
     - MT4 EA是否已加载并运行
     - 两端EA的`InpSymbol`参数是否与输入的值一致

### 注意事项

1. **逻辑品种必须与EA参数一致**：
   - A端EA的`InpSymbol`参数
   - B端EA的`InpSymbol`参数
   - 总控程序的逻辑品种
   - 三者必须完全一致（大小写敏感）

2. **修改逻辑品种后**：
   - 共享内存连接会断开并重新连接
   - 界面会重新读取数据
   - 如果EA未运行或参数不匹配，会显示错误

3. **日志乱码**：
   - 已修复的关键日志现在使用英文
   - 如果仍有乱码，可能是其他日志中的中文字符
   - 建议将MQL4文件保存为UTF-8编码（如果MetaEditor支持）

## 🔧 编译和部署

### 编译

1. **重新编译GUI程序**：
   - 打开Visual Studio
   - 生成 → 生成解决方案（或按`F7`）

2. **重新编译EA**（如果需要）：
   - 在MetaEditor中打开`HedgeArbEA.mq4`
   - 按`F7`编译

### 部署

1. **部署GUI程序**：
   - 将编译后的`HedgeArbControlProgramGUI.exe`复制到目标位置
   - 确保`HedgeArbSharedMemory.dll`在同一目录或系统PATH中

2. **部署EA**（如果需要）：
   - 将编译后的`HedgeArbEA.ex4`复制到MT4的`MQL4\Experts\`目录
   - 将`HedgeArbSharedMemory.dll`复制到MT4的`MQL4\Libraries\`目录

## 📝 验证修复

### 验证日志修复

1. **启动EA**（两端）
2. **查看MT4日志**：
   - 应该看到：`[SendCommand] Command sent to shared memory: OPEN;A;B`
   - 不应该看到：`？？？？？？？？？？？`

### 验证GUI功能

1. **启动总控程序**
2. **修改逻辑品种**：
   - 在输入框中输入`XAUUSD`
   - 点击"应用"
   - 应该看到成功消息或错误消息
3. **检查状态栏**：
   - 应该显示新的逻辑品种名称

## 🔍 如果仍有问题

### 日志仍然乱码

1. **检查文件编码**：
   - 在MetaEditor中，文件 → 另存为
   - 选择UTF-8编码（如果支持）

2. **检查MT4设置**：
   - 工具 → 选项 → 服务器
   - 查看语言设置

### GUI无法修改逻辑品种

1. **检查输入框是否可见**：
   - 窗口大小是否足够
   - 控件是否被遮挡

2. **检查按钮点击**：
   - 点击"应用"按钮后，是否有任何反应
   - 查看是否有错误消息

3. **检查共享内存连接**：
   - 修改后，状态栏是否更新
   - 数据是否正常显示


