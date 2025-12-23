# 安装 Visual Studio 指南

## 📥 下载 Visual Studio

1. **访问下载页面**：
   - https://visualstudio.microsoft.com/zh-hans/downloads/
   - 或搜索 "Visual Studio Community 下载"

2. **选择版本**：
   - **Visual Studio Community**（免费，推荐）
   - 或 Visual Studio Professional（付费）

3. **点击"免费下载"**

---

## 🔧 安装步骤

### 步骤1：运行安装程序

1. 运行下载的安装程序（如 `vs_community.exe`）

### 步骤2：选择工作负载

1. 在"工作负载"选项卡中，勾选：
   - ✅ **使用C++的桌面开发**
   - 这会自动安装：
     - MSVC编译器
     - Windows SDK
     - CMake工具（可选）

2. 点击"安装"

### 步骤3：等待安装完成

- 安装可能需要30分钟到1小时
- 需要几GB的磁盘空间

---

## ✅ 安装完成后

1. **启动 Visual Studio**
2. **登录**（可选，可以跳过）
3. **选择主题**（深色/浅色）
4. **现在你可以创建项目了**：
   - 按 `Ctrl+Shift+N`
   - 或：文件 → 新建 → 项目

---

## 🎯 验证安装

1. 打开 Visual Studio
2. 按 `Ctrl+Shift+N` 创建新项目
3. 应该能看到：
   - **Visual C++** → **Windows桌面** → **动态链接库(DLL)**
   - **Visual C++** → **Windows桌面** → **控制台应用程序**

如果能看到这些选项，说明安装成功！

---

## 📝 最小安装（如果磁盘空间有限）

如果只需要编译C++项目，可以：

1. 在安装程序中选择"单个组件"
2. 勾选：
   - ✅ MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - ✅ Windows 10/11 SDK
   - ✅ C++ CMake 工具（可选）

---

## ⚠️ 注意事项

- Visual Studio 和 Visual Studio Code 是两个不同的软件
- Visual Studio 比较大（几GB），需要时间下载和安装
- 安装后，你可以继续使用 VS Code 编辑代码，但用 Visual Studio 编译

---

## 🚀 安装完成后

安装完成后，按照 `Visual Studio编译详细步骤.md` 中的步骤创建项目即可。


