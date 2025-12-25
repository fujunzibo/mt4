# DLL字符串传递和乱码修复说明

## 🔍 问题分析

### 问题1：DLL诊断代码读取失败

**现象**：
- EA日志显示：`code='?????T' codeLen=6`
- DLL日志显示：`code="DLL_OK_INIT"`（11字节）
- EA无法正确读取DLL写入的诊断代码

**原因**：
- `GetCommand`函数的字符串复制逻辑过于复杂
- 尝试检测MQL4字符串结构体，但检测逻辑可能不准确
- 导致字符串没有正确复制到MQL4缓冲区

### 问题2：中文日志乱码

**现象**：
- MT4日志中中文显示为`????????`
- 影响调试和问题排查

**原因**：
- MT4的日志系统对中文支持不好
- 需要使用英文日志

---

## ✅ 修复方案

### 修复1：简化`GetCommand`字符串复制逻辑

**文件**：`HedgeArbSharedMemory.cpp`

**修改前**：
- 复杂的MQL4字符串结构体检测逻辑
- 尝试提取结构体中的`str`指针
- 多个分支处理，容易出错

**修改后**：
```cpp
if (code)
{
    // Find source string length
    size_t srcLen = 0;
    while (srcLen < sizeof(g_pSharedData->command.code) && 
           g_pSharedData->command.code[srcLen] != '\0')
        srcLen++;
    
    if (srcLen > 0)
    {
        // Copy string directly to MQL4 buffer (MQL4 provides 256 bytes buffer)
        // Use strncpy_s for safety (ensures null termination)
        size_t maxCopy = (srcLen < 255) ? srcLen : 255;
        strncpy_s(code, 256, g_pSharedData->command.code, maxCopy);
        code[maxCopy] = '\0';  // Ensure null termination
        
        char copyMsg[256];
        _snprintf_s(copyMsg, sizeof(copyMsg), _TRUNCATE,
            "[GetCommand] Copied code: len=%zu content=\"%s\"", 
            maxCopy, code);
        AppendLogLine(copyMsg);
    }
    else
    {
        // Source is empty, clear output
        code[0] = '\0';
        AppendLogLine("[GetCommand] Source code is empty, cleared output");
    }
}
```

**优势**：
- ✅ 简单直接，使用`strncpy_s`安全复制
- ✅ 确保null终止符
- ✅ 不依赖复杂的结构体检测
- ✅ MQL4的`string &`参数实际上就是`char*`缓冲区

---

### 修复2：修复中文乱码

**文件**：`HedgeArbEA.mq4`

**修改内容**：

1. **初始化日志**：
   ```mql4
   // 修改前
   Print("HedgeArbEA 初始化完成。BrokerId=", ...);
   
   // 修改后
   Print("[EA Init] HedgeArbEA initialized. BrokerId=", ...);
   ```

2. **定时器日志**：
   ```mql4
   // 修改前
   Print("【成功】定时器模式已启用...");
   Print("【提示】未启用定时器模式...");
   
   // 修改后
   Print("[Timer] SUCCESS: Timer mode enabled...");
   Print("[Timer] Timer mode not enabled...");
   ```

3. **去初始化日志**：
   ```mql4
   // 修改前
   Print("共享内存连接已关闭。");
   Print("HedgeArbEA 结束，BrokerId=", ...);
   
   // 修改后
   Print("[EA Deinit] Shared memory connection closed.");
   Print("[EA Deinit] HedgeArbEA ended. BrokerId=", ...);
   ```

---

## 📋 验证步骤

### 1. 重新编译DLL

```
1. 打开Visual Studio
2. 打开 mt4.sln
3. 生成 → 重新生成解决方案
4. 确认编译成功（无错误）
```

### 2. 重新编译EA

```
1. 打开MetaEditor
2. 打开 HedgeArbEA.mq4
3. 按 F7 编译
4. 确认编译成功（无错误）
```

### 3. 重新部署

```
1. 将 DLL 复制到 MT4 Libraries 目录
2. 完全关闭MT4（确保DLL被卸载）
3. 重新启动MT4
```

### 4. 测试

```
1. 在MT4中加载EA
2. 查看EA日志，应该看到：
   [EA Init] HedgeArbEA initialized. BrokerId=B ...
   [DLL_DIAG] SUCCESS: DLL is working correctly! Diagnostic code=DLL_OK_INIT
3. 查看debugView日志，确认DLL正确写入和读取
```

---

## ✅ 预期结果

### EA日志（成功）：
```
[EA Init] HedgeArbEA initialized. BrokerId=B IsMaster=false LogicalSymbol=GOLD TradeSymbol=GOLD UseTimer=false
[InitSharedMemory] Shared memory initialized successfully. Symbol=GOLD
[DLL_DIAG] Attempt 1 (immediate): GetCommand returned=true code='DLL_OK_INIT' codeLen=11
[DLL_DIAG] SUCCESS: DLL is working correctly! Diagnostic code=DLL_OK_INIT
```

### debugView日志（成功）：
```
[HedgeArbDLL] [InitSharedMemory] Diagnostic code written: expected='DLL_OK_INIT' (len=11), verified='DLL_OK_INIT'
[HedgeArbDLL] [GetCommand] Copied code: len=11 content="DLL_OK_INIT"
```

---

## ⚠️ 注意事项

1. **字符串复制**：
   - 使用`strncpy_s`确保安全复制
   - 确保null终止符
   - MQL4缓冲区是256字节，足够大

2. **日志语言**：
   - 所有关键日志使用英文
   - 避免中文乱码问题
   - 便于调试和问题排查

3. **向后兼容**：
   - 修复不影响其他功能
   - `GetCommand`的其他参数（longBroker, shortBroker）使用相同的逻辑

---

## 🔄 如果问题仍然存在

如果修复后问题仍然存在：

1. **检查DLL版本**：
   - 确认DLL文件是最新编译的
   - 检查文件时间戳

2. **检查MT4缓存**：
   - 完全关闭MT4
   - 删除MT4缓存（如果存在）
   - 重新启动MT4

3. **查看debugView日志**：
   - 确认DLL是否正确写入诊断代码
   - 确认`GetCommand`是否正确复制字符串

4. **简化测试**：
   - 暂时禁用诊断检查
   - 直接测试其他功能（如SetQuote/GetQuote）

