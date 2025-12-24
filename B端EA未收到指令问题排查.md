# B端EA未收到指令问题排查

## 🔍 问题现象

**A端EA日志**：
- ✅ 显示 `[SendCommand] Command sent to shared memory: OPEN;B;A`
- ✅ 显示 `[SetGlobalCommand] Command set successfully: OPEN;B;A`
- ✅ 说明A端EA成功将指令写入共享内存

**B端EA日志**：
- ✅ 显示 `[InitSharedMemory] Shared memory initialized successfully. Symbol=GOLD`
- ✅ 一直在执行 `ExecuteMainLogic`
- ❌ **没有看到任何 `[ExecCommands]` 相关的日志**
- ❌ **没有看到任何 `[GetGlobalCommand]` 的错误日志**

## 📋 可能的原因

### 原因1：`GetGlobalCommand`返回空字符串，但没有输出错误日志

**分析**：
- `GetGlobalCommand`在以下情况返回空字符串：
  1. `GetCommand`返回`false`（`g_pSharedData`为NULL或`isActive=false`）
  2. `isActive=false`（指令存在但未激活）
  3. `code`不是"CLOSEALL"或"OPEN"

**当前日志频率**：
- `[GetGlobalCommand] Failed...`：每10次调用输出一次
- `[GetGlobalCommand] Command exists but isActive=false`：每10次调用输出一次
- `[ExecCommands]`：只有在收到指令时才输出

**问题**：如果B端EA刚启动，可能还没到第10次调用，所以看不到错误日志。

### 原因2：共享内存连接问题

**可能情况**：
- B端EA的`g_pSharedData`为NULL
- B端EA和A端EA访问的不是同一个共享内存
- `InpSymbol`不一致

### 原因3：指令被立即清除

**可能情况**：
- A端EA发送指令后，立即被清除
- 指令的`isActive`被设置为`false`

## ✅ 已实施的修复

### 修复1：增强`GetGlobalCommand`的日志

**文件**：`HedgeArbEA.mq4`

**修复内容**：
1. **增强错误日志**：
   - 在`GetCommand`返回`false`时，输出更详细的错误信息
   - 在`isActive=false`时，输出指令的详细信息（code, longBroker, shortBroker, timestamp）

2. **增加成功日志**：
   - 成功获取到激活的指令时，也输出日志（每100次一次）

### 修复2：增强`ExecCommands`的日志

**文件**：`HedgeArbEA.mq4`

**修复内容**：
1. **增加无指令日志**：
   - 即使没有指令，也输出日志（每100次一次）
   - 显示`BrokerId`、`Symbol`、`Last executed command`

## 📋 下一步操作

### 步骤1：重新编译EA

1. 打开MetaEditor
2. 打开 `HedgeArbEA.mq4`
3. 编译（或按`F7`）
4. 确认编译成功

### 步骤2：重新加载EA（两端都要做）

1. **A端**：
   - 从图表移除EA
   - 重新加载EA

2. **B端**：
   - 从图表移除EA
   - 重新加载EA

### 步骤3：观察日志

**B端EA应该看到以下日志之一**：

1. **如果`GetCommand`返回`false`**（每10次一次）：
   ```
   [GetGlobalCommand] Failed to get command from shared memory, Error count: 10
   BrokerId=B Symbol=GOLD g_pSharedData is NULL or GetCommand returned false
   ```

2. **如果`isActive=false`**（每10次一次）：
   ```
   [GetGlobalCommand] Command exists but isActive=false, Inactive count: 10
   BrokerId=B Symbol=GOLD Code='OPEN' LongBroker='B' ShortBroker='A' Timestamp=...
   ```

3. **如果没有指令**（每100次一次）：
   ```
   [ExecCommands] No command found, No command count: 100
   BrokerId=B Symbol=GOLD Last executed: ''
   ```

4. **如果成功收到指令**：
   ```
   [ExecCommands] BrokerId=B InpSymbol=GOLD Received command: 'OPEN;B;A' (from shared memory)
   ```

## 🔍 如果问题仍然存在

### 检查清单

1. **确认`InpSymbol`一致**：
   - [ ] A端EA的`InpSymbol`参数值是什么？
   - [ ] B端EA的`InpSymbol`参数值是什么？
   - [ ] 两者是否完全一致（包括大小写）？

2. **确认共享内存名称**：
   - 共享内存名称格式：`HedgeArbEA_SharedMemory_{InpSymbol}`
   - 例如：如果`InpSymbol=GOLD`，共享内存名称是`HedgeArbEA_SharedMemory_GOLD`
   - A端和B端必须使用相同的`InpSymbol`，才能访问同一个共享内存

3. **检查DLL版本**：
   - [ ] A端MT4的`MQL4\Libraries\HedgeArbSharedMemory.dll`是否是最新版本？
   - [ ] B端MT4的`MQL4\Libraries\HedgeArbSharedMemory.dll`是否是最新版本？
   - [ ] 两个DLL文件的时间戳是否一致？

4. **检查EA版本**：
   - [ ] A端EA是否是最新编译的版本？
   - [ ] B端EA是否是最新编译的版本？

5. **查看完整日志**：
   - 等待至少10次`ExecuteMainLogic`执行
   - 应该看到`[GetGlobalCommand]`的错误日志（如果`GetCommand`返回`false`或`isActive=false`）
   - 或者看到`[ExecCommands] No command found`日志（如果没有指令）

## 📝 如果看到`[GetGlobalCommand] Failed...`日志

**说明**：`GetCommand`返回`false`，可能原因：
1. `g_pSharedData`为NULL（共享内存未正确初始化）
2. 共享内存连接失败

**解决方法**：
1. 检查B端EA的初始化日志，确认`[InitSharedMemory] Shared memory initialized successfully`
2. 检查`InpSymbol`是否一致
3. 重新加载B端EA

## 📝 如果看到`[GetGlobalCommand] Command exists but isActive=false`日志

**说明**：指令存在，但`isActive=false`。

**可能原因**：
1. A端EA发送指令后，`isActive`被设置为`false`
2. 指令被其他进程清除

**解决方法**：
1. 检查A端EA的`SetCommand`调用是否成功
2. 检查是否有其他进程在清除指令
3. 检查`SetCommand`的实现，确认`isActive`被正确设置为`true`

## 📝 如果看到`[ExecCommands] No command found`日志

**说明**：`GetGlobalCommand`返回空字符串，但没有错误。

**可能原因**：
1. A端EA还没有发送指令
2. 指令已经被清除
3. `code`不是"CLOSEALL"或"OPEN"

**解决方法**：
1. 等待A端EA发送指令
2. 检查A端EA的日志，确认指令已发送
3. 检查`GetGlobalCommand`的实现，确认指令格式正确

## 📞 如果问题仍然存在

请提供：
1. **A端完整日志**（包含初始化日志和指令发送日志）
2. **B端完整日志**（包含初始化日志和至少10次`ExecuteMainLogic`执行后的日志）
3. **EA参数设置**（特别是`InpSymbol`的值）

这样我可以进一步排查问题。



