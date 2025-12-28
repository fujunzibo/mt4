# InitSharedMemory字符串传递问题修复说明

## 🔍 问题分析

### 问题现象

**debugView日志显示**：
```
[GetCommand] sym=G code="DLL_OK_INIT" long="" short="" ts=0 active=0 hasCode=1
```

**问题**：
- `sym=G`（错误，应该是`GOLD`）
- `code="DLL_OK_INIT"`（正确）

### 根本原因

**MQL4字符串传递问题**：
- EA调用`InitSharedMemory(InpSymbol)`，其中`InpSymbol="GOLD"`
- 但DLL的`InitSharedMemory`函数只收到第一个字符`"G"`
- 导致`g_symbolName = "G"`而不是`"GOLD"`
- 但诊断代码`DLL_OK_INIT`是在DLL内部写入的，不依赖symbol参数，所以是正确的

**影响**：
- 共享内存名称错误：`HedgeArbEA_SharedMemory_G`而不是`HedgeArbEA_SharedMemory_GOLD`
- 如果两个EA使用不同的symbol（一个`G`，一个`GOLD`），它们会连接到不同的共享内存
- 导致A端和B端无法通信

---

## ✅ 修复方案

### 修复：在`InitSharedMemory`中处理MQL4字符串结构体

**文件**：`HedgeArbSharedMemory.cpp`

**修复逻辑**：
1. 检测如果只收到1个字符，可能是MQL4字符串结构体问题
2. 尝试读取MQL4字符串结构体：`{ int len; int reserved; char* str; }`
3. 如果检测到结构体，提取`str`指针并读取完整字符串
4. 如果检测失败，使用直接字符串（向后兼容）

**代码修改**：
```cpp
// CRITICAL: MQL4 string may be passed as structure or direct pointer
size_t symbolLen = strlen(symbol);

// If we only got 1 character, it might be MQL4 string structure issue
if (symbolLen == 1 && symbol[0] != '\0')
{
    const int* structPtr = reinterpret_cast<const int*>(symbol);
    int mqlLen = structPtr[0];
    int mqlReserved = structPtr[1];
    
    // Check if this looks like a structure (len > 1, reserved is 0)
    if (mqlLen > 1 && mqlLen <= 256 && mqlReserved == 0)
    {
        // Try to get the actual string pointer
        char* nonConstSymbol = const_cast<char*>(symbol);
        const char** strPtrPtr = reinterpret_cast<const char**>(nonConstSymbol + 8);
        const char* actualStr = *strPtrPtr;
        
        if (actualStr != NULL && actualStr != symbol && actualStr != (symbol + 8))
        {
            // Use the string from structure
            g_symbolName.assign(actualStr, mqlLen);
        }
    }
}
else
{
    // Normal case: direct string with length > 1
    g_symbolName = symbol;
}
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

### 2. 重新部署DLL

```
1. 将 DLL 复制到 MT4 Libraries 目录
2. 完全关闭MT4（确保旧DLL被卸载）
3. 重新启动MT4
```

### 3. 测试

**EA日志**（应该看到）：
```
[InitSharedMemory] Shared memory initialized successfully. Symbol=GOLD
```

**debugView日志**（应该看到）：
```
[HedgeArbDLL] [InitSharedMemory] Using symbol: 'GOLD' (len=4)
[HedgeArbDLL] [InitSharedMemory] Using mapName: 'HedgeArbEA_SharedMemory_GOLD'
[HedgeArbDLL] [GetCommand] sym=GOLD code="DLL_OK_INIT" ...
```

---

## ⚠️ 重要说明

### 为什么code正确但sym错误？

**原因**：
- `code="DLL_OK_INIT"`是在DLL内部硬编码写入的，不依赖symbol参数
- `sym=G`是因为`g_symbolName`在`InitSharedMemory`中从symbol参数设置，但只收到第一个字符

**影响**：
- 如果两个EA的symbol不一致（一个`G`，一个`GOLD`），它们会连接到不同的共享内存
- 导致A端和B端无法通信
- **必须修复**，否则系统无法正常工作

---

## 🔄 如果问题仍然存在

如果修复后`sym`仍然是`G`：

1. **检查EA参数**：
   - 确认`InpSymbol="GOLD"`（不是`"G"`）
   - 查看EA日志，确认EA传递的值

2. **查看debugView日志**：
   - 查看`[InitSharedMemory]`相关的日志
   - 确认DLL接收到的symbol参数

3. **使用文件传递方案**（如果结构体检测失败）：
   - EA将symbol写入文件
   - DLL从文件读取
   - 完全避免MQL4字符串传递问题

---

## 📝 总结

1. **问题**：`InitSharedMemory`只收到第一个字符`"G"`而不是`"GOLD"`
2. **原因**：MQL4字符串传递问题，与`GetCommand`的问题相同
3. **修复**：在`InitSharedMemory`中检测并处理MQL4字符串结构体
4. **影响**：如果不修复，A端和B端会连接到不同的共享内存，无法通信


