# InitSharedMemory函数修复说明

## 🔍 问题分析

**问题**：B端EA的`InitSharedMemory`失败，但没有显示错误信息

**根本原因**：
`GetLastError()`的使用不正确。在Windows API中，`GetLastError()`只在特定情况下有效：

1. **如果先调用`OpenFileMapping`成功**：
   - `GetLastError()`可能返回之前的值，不一定是`ERROR_ALREADY_EXISTS`
   - 无法准确判断共享内存是否已存在

2. **如果`CreateFileMapping`成功**：
   - `GetLastError()`会返回`ERROR_ALREADY_EXISTS`（如果共享内存已存在）
   - 但必须在`CreateFileMapping`之后立即调用才有效

**问题代码**：
```cpp
// 尝试打开已存在的共享内存
g_hMapFile = OpenFileMappingA(...);

if (g_hMapFile == NULL) {
    // 创建新的共享内存
    g_hMapFile = CreateFileMappingA(...);
}

// 映射到进程地址空间
g_pSharedData = (SharedData*)MapViewOfFile(...);

// 问题：GetLastError()在这里调用，可能返回之前的值
if (GetLastError() != ERROR_ALREADY_EXISTS) {
    memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);  // 可能误判！
}
```

**问题场景**：
1. **A端EA先启动**：
   - A端EA创建共享内存并初始化
   - B端EA启动，`OpenFileMapping`成功（共享内存已存在）
   - `GetLastError()`可能不是`ERROR_ALREADY_EXISTS`
   - 误判为新创建的，执行`memset`，清空A端已写入的数据！

2. **主控程序先启动**：
   - 主控程序创建共享内存（但不初始化）
   - B端EA启动，`OpenFileMapping`成功
   - `GetLastError()`可能不是`ERROR_ALREADY_EXISTS`
   - 误判为新创建的，执行`memset`，清空数据

## ✅ 修复方案

**原则**：只有在真正新创建共享内存时才初始化数据结构。

**修复后的代码**：
```cpp
bool InitSharedMemory(const char* symbol) {
    // ... 关闭旧的连接 ...
    
    // 先尝试打开已存在的共享内存
    g_hMapFile = OpenFileMappingA(...);
    
    bool isNewlyCreated = false;
    
    if (g_hMapFile == NULL) {
        // 如果不存在，创建新的共享内存
        SetLastError(0);  // 清除之前的错误码
        g_hMapFile = CreateFileMappingA(...);
        
        if (g_hMapFile == NULL) {
            return false;
        }
        
        // 检查是否是新建的（必须在CreateFileMapping后立即调用）
        DWORD lastError = GetLastError();
        isNewlyCreated = (lastError != ERROR_ALREADY_EXISTS);
    }
    // 如果OpenFileMapping成功，说明共享内存已存在，isNewlyCreated保持false
    
    // 映射到进程地址空间
    g_pSharedData = (SharedData*)MapViewOfFile(...);
    
    if (g_pSharedData == NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    
    // ==== 修复：只有在真正新创建时才初始化数据结构 ====
    if (isNewlyCreated) {
        memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);
        g_pSharedData->version = 0;
        g_pSharedData->lockFlag = 0;
    }
    
    return true;
}
```

**修复要点**：
1. **使用`isNewlyCreated`标志**：明确记录共享内存是否是新创建的
2. **在`CreateFileMapping`之前调用`SetLastError(0)`**：清除之前的错误码
3. **在`CreateFileMapping`之后立即调用`GetLastError()`**：获取准确的错误码
4. **如果`OpenFileMapping`成功**：说明共享内存已存在，`isNewlyCreated`保持false
5. **只有在`isNewlyCreated == true`时才初始化**：避免清空已存在的数据

## 📋 下一步操作

### 步骤1：重新编译DLL

1. 打开Visual Studio
2. 打开 `mt4.sln` 解决方案
3. 生成 → 生成解决方案（或按`F7`）
4. 确认编译成功

### 步骤2：重新部署DLL

1. 将编译后的 `HedgeArbSharedMemory.dll` 复制到：
   - A端MT4的 `MQL4\Libraries\` 目录
   - B端MT4的 `MQL4\Libraries\` 目录

2. **重要**：必须重新加载EA才能使用新的DLL

### 步骤3：重新加载EA（两端都要做）

1. **A端**：
   - 从图表移除EA
   - 重新加载EA
   - 确认显示"共享内存连接初始化成功。"

2. **B端**：
   - 从图表移除EA
   - 重新加载EA
   - 确认显示"共享内存连接初始化成功。"

### 步骤4：测试

1. **等待A端发送指令**
2. **查看B端日志**：
   - 如果成功：应该看到 `[ExecCommands] BrokerId=B InpSymbol=GOLD Received command: 'OPEN;B;A'`
   - 如果失败但指令存在：应该看到 `[GetGlobalCommand] Command exists but isActive=false`
   - 如果共享内存未初始化：应该看到 `[GetGlobalCommand] Failed to get command from shared memory`

## 🔍 验证修复

### 测试场景1：A端先启动，B端后启动

1. 启动A端EA
2. 等待A端发送指令
3. 启动B端EA
4. **预期**：B端应该收到指令，不应该清空A端已写入的数据

### 测试场景2：B端先启动，A端后启动

1. 启动B端EA
2. 启动A端EA
3. 等待A端发送指令
4. **预期**：B端应该收到指令

### 测试场景3：主控程序先启动

1. 启动主控程序
2. 启动A端EA和B端EA
3. 等待A端发送指令
4. **预期**：B端应该收到指令，主控程序不应该清空数据

## 📝 修复效果

修复后：
- ✅ B端EA可以正确初始化共享内存
- ✅ 不会清空A端已写入的数据
- ✅ 不会清空主控程序已写入的数据
- ✅ 只有在真正新创建时才初始化数据结构

## 🔧 如果问题仍然存在

请检查：

1. **DLL是否重新编译**：
   - [ ] 是否使用了最新编译的 `HedgeArbSharedMemory.dll`？
   - [ ] DLL文件时间戳是否是最新的？

2. **DLL是否重新部署**：
   - [ ] A端MT4的 `MQL4\Libraries\HedgeArbSharedMemory.dll` 是否已更新？
   - [ ] B端MT4的 `MQL4\Libraries\HedgeArbSharedMemory.dll` 是否已更新？

3. **EA是否重新加载**：
   - [ ] A端EA是否显示"共享内存连接初始化成功。"？
   - [ ] B端EA是否显示"共享内存连接初始化成功。"？

4. **日志输出**：
   - [ ] B端是否看到 `[GetGlobalCommand] Failed...` 或 `[GetGlobalCommand] Command exists but isActive=false`？
   - [ ] 如果看到，错误信息是什么？

## 📞 如果问题仍然存在

请提供：
1. **A端完整日志**（包含初始化日志）
2. **B端完整日志**（包含初始化日志）
3. **DLL文件信息**（文件大小、修改时间）

这样我可以进一步排查问题。


