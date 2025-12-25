# GUI编译路径问题修复说明

## 🔍 问题

**编译错误**：
```
error C1083: 无法打开包括文件: "../HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h": No such file or directory
```

**原因**：
- GUI项目位置：`HedgeArbEA\HedgeArbControlProgramGUI\`
- 头文件位置：`HedgeArbEA\HedgeArbSharedMemory_New\HedgeArbSharedMemory.h`
- 错误的相对路径：`../HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h`
- 正确的相对路径：`../HedgeArbSharedMemory_New/HedgeArbSharedMemory.h`

---

## ✅ 修复

**修改文件**：`HedgeArbControlProgramGUI.cpp`

**修改前**：
```cpp
#include "../HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h"
```

**修改后**：
```cpp
#include "../HedgeArbSharedMemory_New/HedgeArbSharedMemory.h"
```

---

## 📋 目录结构

```
D:\mt4\
├── HedgeArbEA\
│   ├── HedgeArbControlProgramGUI\     ← GUI项目位置
│   │   └── HedgeArbControlProgramGUI.cpp
│   └── HedgeArbSharedMemory_New\       ← 头文件位置
│       └── HedgeArbSharedMemory.h
```

**相对路径计算**：
- 从 `HedgeArbEA\HedgeArbControlProgramGUI\` 
- 到 `HedgeArbEA\HedgeArbSharedMemory_New\`
- 路径：`../HedgeArbSharedMemory_New/`（上一级，然后进入HedgeArbSharedMemory_New）

---

## ✅ 验证

重新编译GUI项目，应该能够成功找到头文件。

---

## 🔄 如果仍有问题

如果修复后仍有路径问题，可以：

1. **在项目属性中配置包含路径**：
   - 右键项目 → 属性 → C/C++ → 常规 → 附加包含目录
   - 添加：`$(ProjectDir)..\HedgeArbSharedMemory_New`

2. **使用绝对路径**（不推荐）：
   ```cpp
   #include "D:/mt4/HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h"
   ```

3. **复制头文件到项目目录**（不推荐）：
   - 将头文件复制到GUI项目目录
   - 使用 `#include "HedgeArbSharedMemory.h"`

---

## 📝 总结

- ✅ 已修复头文件包含路径
- ✅ 从 `../HedgeArbEA/...` 改为 `../HedgeArbSharedMemory_New/...`
- ✅ 现在应该可以正常编译

