# HedgeArbControlProgramGUI 部署指南

## 📋 部署前检查

### 1. 确认编译成功

编译完成后，检查以下文件是否存在：

**DLL文件**（必须）：
- `D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.dll`
- `D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.lib`

**GUI程序**：
- `D:\mt4\HedgeArbEA\Release\HedgeArbControlProgramGUI.exe`

**控制台程序**（可选，如果还需要）：
- `D:\mt4\HedgeArbEA\Release\HedgeArbControlProgram.exe`

### 2. 确认文件完整性

- DLL文件大小：应该有几KB到几十KB
- EXE文件大小：应该有几十KB到几百KB
- 右键文件 → 属性 → 详细信息，应该显示"32位"

---

## 🚀 部署步骤

### 步骤1：编译MQL4 EA（必须！）

**重要**：DLL和EX4文件都需要！

1. **打开MetaEditor**
   - 在MT4中按 `F4` 键
   - 或：工具 → MetaQuotes语言编辑器

2. **打开EA文件**
   - 在导航器中找到 `HedgeArbEA.mq4`
   - 双击打开

3. **编译EA**
   - 按 `F7` 键
   - 或点击工具栏的"编译"按钮

4. **检查编译结果**
   - 底部"工具"窗口应显示：`0 error(s), 0 warning(s)`
   - 如果成功，`.ex4` 文件会自动生成在 `Experts` 目录

5. **验证文件**
   - 检查 `MQL4\Experts\` 目录下是否有 `HedgeArbEA.ex4`

---

### 步骤2：部署DLL到MT4

1. **找到MT4的Libraries目录**

   方法A：标准安装路径
   ```
   C:\Program Files\MetaTrader 4\MQL4\Libraries\
   ```

   方法B：用户数据路径（更常见）
   ```
   C:\Users\你的用户名\AppData\Roaming\MetaQuotes\Terminal\
   ```
   然后找到你的MT4实例文件夹（通常是一串字符），进入：
   ```
   [实例文件夹]\MQL4\Libraries\
   ```

2. **复制DLL文件**

   - 复制：`D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.dll`
   - 粘贴到：MT4的 `MQL4\Libraries\` 目录

3. **验证DLL已部署**

   - 在MT4中：工具 → 选项 → 专家顾问
   - 勾选"允许DLL导入"
   - 重启MT4（如果已打开）

**注意**：DLL和EX4文件都需要！DLL提供共享内存功能，EX4是EA的主程序。

---

### 步骤2：配置GUI程序

1. **编辑配置文件**（如果需要修改）

   打开 `HedgeArbControlProgramGUI.cpp`，修改以下配置：

   ```cpp
   // 配置
   const char* SYMBOL_NAME = "GOLD";  // 改为你使用的品种
   const char* DATA_DIR = "D:\\mt4\\test\\";  // 改为你的数据目录
   const int UPDATE_INTERVAL_MS = 100;  // 界面更新间隔（毫秒）
   ```

   修改后需要重新编译。

2. **确认数据目录存在**

   确保 `DATA_DIR` 指向的目录存在：
   - 默认：`D:\mt4\test\`
   - 如果不存在，创建该目录

---

### 步骤3：运行GUI程序

#### 方法1：直接运行

1. 双击 `HedgeArbControlProgramGUI.exe`
2. 程序会自动：
   - 连接共享内存
   - 显示实时报价、指令状态、盈亏信息、历史记录

#### 方法2：从Visual Studio运行

1. 在Visual Studio中，右键 `HedgeArbControlProgramGUI` 项目
2. 选择"设为启动项目"
3. 按 `F5` 运行

#### 方法3：创建快捷方式

1. 右键 `HedgeArbControlProgramGUI.exe` → 创建快捷方式
2. 将快捷方式放到桌面或开始菜单
3. 方便以后快速启动

---

### 步骤4：启动MT4并加载EA

1. **启动MT4 A端**

   - 打开MT4 A端
   - 在图表上加载 `HedgeArbEA.mq4`
   - 设置参数：
     - `InpBrokerId = "A"`
     - `InpIsMaster = true`
     - `InpSymbol = "GOLD"`（与GUI程序配置一致）

2. **启动MT4 B端**

   - 打开MT4 B端
   - 在图表上加载 `HedgeArbEA.mq4`
   - 设置参数：
     - `InpBrokerId = "B"`
     - `InpIsMaster = false`
     - `InpSymbol = "GOLD"`（与GUI程序配置一致）

3. **验证连接**

   - 查看EA日志，应该看到：
     ```
     共享内存连接初始化成功。
     ```
   - 如果看到错误，检查：
     - DLL是否在正确位置
     - MT4是否允许DLL导入
     - GUI程序是否正在运行

---

## ✅ 验证部署成功

### 1. 检查GUI程序界面

程序启动后，应该显示：

- **实时报价区域**（左上）：
  - Broker A 的 Bid、Ask、中间价
  - Broker B 的 Bid、Ask、中间价
  - 价差（A-B）

- **指令状态区域**（右上）：
  - 当前活动指令（OPEN、CLOSEALL等）
  - 做多端和做空端
  - 两个Broker的确认状态

- **盈亏信息区域**（左下）：
  - Broker A 和 B 的盈亏（点和USD）
  - 合计盈亏

- **历史记录区域**（右下）：
  - 最近20条交易记录
  - 显示时间、Broker、订单号、类型、盈亏

- **状态栏**（底部）：
  - 显示数据版本号和品种名称

### 2. 检查数据更新

- 界面应该每100毫秒自动刷新
- 报价数据应该实时更新
- 如果有持仓，盈亏信息应该显示

### 3. 检查文件生成

在 `DATA_DIR` 目录下，应该能看到以下文件：

- `ArbPrice_GOLD_A.csv` - A端报价
- `ArbPrice_GOLD_B.csv` - B端报价
- `ArbState_Account_GOLD_A.csv` - A端账户信息
- `ArbState_Account_GOLD_B.csv` - B端账户信息
- `ArbState_Positions_GOLD_A.csv` - A端持仓
- `ArbState_Positions_GOLD_B.csv` - B端持仓
- `ArbState_History_GOLD_A.csv` - A端历史记录
- `ArbState_History_GOLD_B.csv` - B端历史记录
- `ArbState_GOLD_A_profitUSD.csv` - A端盈亏
- `ArbState_GOLD_B_profitUSD.csv` - B端盈亏

---

## 🔧 常见问题排查

### 问题1：GUI程序启动后显示"无法初始化共享内存"

**可能原因**：
- MT4 EA还没有加载
- DLL未正确部署
- 品种名称不匹配

**解决方法**：
1. 确保MT4已加载EA
2. 检查DLL是否在 `MQL4\Libraries\` 目录
3. 检查GUI程序和EA中的品种名称是否一致
4. 重启MT4和GUI程序

### 问题2：界面不更新

**可能原因**：
- 共享内存连接失败
- MT4 EA未运行
- 数据目录不存在

**解决方法**：
1. 检查MT4 EA是否正常运行
2. 查看EA日志，确认共享内存连接成功
3. 检查数据目录是否存在
4. 重启GUI程序

### 问题3：显示的数据都是"-"

**可能原因**：
- 数据文件不存在
- 文件格式不正确
- 品种名称不匹配

**解决方法**：
1. 检查数据文件是否存在
2. 确认品种名称配置正确
3. 等待EA生成数据文件

### 问题4：程序无法启动

**可能原因**：
- 缺少依赖库
- 文件损坏
- 权限问题

**解决方法**：
1. 检查是否在32位系统上运行（MT4是32位）
2. 以管理员身份运行
3. 检查杀毒软件是否拦截
4. 重新编译程序

---

## 📝 运行顺序建议

### 推荐的启动顺序：

1. **启动GUI程序**
   - 先启动GUI程序，它会创建共享内存

2. **启动MT4 A端**
   - 打开MT4 A端
   - 加载EA，设置参数

3. **启动MT4 B端**
   - 打开MT4 B端
   - 加载EA，设置参数

4. **验证连接**
   - 查看GUI程序界面是否显示数据
   - 查看MT4 EA日志确认连接成功

### 关闭顺序：

1. 先关闭MT4（两个实例）
2. 再关闭GUI程序

---

## 🎯 使用建议

### 1. 窗口布局

- GUI程序窗口可以调整大小
- 建议分辨率：至少 800x600
- 可以最大化窗口查看详细信息

### 2. 监控要点

- **实时报价**：关注价差变化
- **指令状态**：监控开仓/平仓指令执行情况
- **盈亏信息**：实时查看盈亏变化
- **历史记录**：查看交易历史

### 3. 数据备份

- 定期备份 `DATA_DIR` 目录下的CSV文件
- 这些文件包含所有交易记录和账户信息

---

## 🔄 更新程序

如果需要更新程序：

1. **停止运行**
   - 关闭GUI程序
   - 关闭MT4（两个实例）

2. **替换文件**
   - 用新编译的 `HedgeArbControlProgramGUI.exe` 替换旧文件
   - **重要**：如果DLL也重新编译了，必须同时替换 `HedgeArbSharedMemory.dll`
   - DLL位置：`MQL4\Libraries\HedgeArbSharedMemory.dll`
   - 源文件：`D:\mt4\HedgeArbEA\Release\HedgeArbSharedMemory.dll`

3. **版本匹配**
   - **重要**：GUI程序和MT4 EA必须使用**相同版本**的DLL
   - 不能混用新旧版本

4. **重新启动**
   - 按照启动顺序重新启动
   - 建议重启MT4以确保加载新版本的DLL

---

## 📞 故障排除

如果遇到问题：

1. **查看日志**
   - MT4 EA日志
   - Windows事件查看器

2. **检查文件**
   - 确认所有文件都在正确位置
   - 检查文件权限

3. **重启服务**
   - 重启MT4
   - 重启GUI程序
   - 必要时重启电脑

---

## ✅ 部署检查清单

- [ ] DLL已复制到MT4的 `MQL4\Libraries\` 目录
- [ ] MT4已启用"允许DLL导入"
- [ ] GUI程序配置正确（品种名称、数据目录）
- [ ] 数据目录存在
- [ ] GUI程序可以正常启动
- [ ] MT4 A端和B端都已加载EA
- [ ] EA参数配置正确
- [ ] GUI程序界面显示数据
- [ ] 数据文件正常生成

完成以上所有检查项后，部署就完成了！

