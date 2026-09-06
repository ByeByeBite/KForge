# KForge

> C++20 模块化基础工具库 + 算法 / C++ 学习项目。

KForge 是一个个人 C++20 学习项目，既是「自带库」（日志、配置解析、命令行交互、计时、大数运算），也是算法与 C++ 特性的实验场。库采用 **C++20 具名模块（Modules）** 组织，经过模块化重构后不再依赖旧的单头文件结构。

---

## 特性一览

| 模块 | 作用 |
|------|------|
| `klogger` | 等级日志、错误码、VT100 颜色常量 |
| `kson` | 类 JSON 自定义配置格式（节点树、路径访问、`.kson` 文件读取） |
| `kcli` | 链式彩色输出 `kout/kin`、标题框、选项菜单 |
| `kutil` | 可视化工具（迷宫/柱状图打印、控制台尺寸适配） |
| `ktimer` | 计时器管理 |
| `kbignum` | 大整数/大小数/分数/复数（`BigInt`/`BigDec`/`BigFrc`/`BigCpx`），乘法自动 NTT |

---

## 环境要求

| 依赖 | 说明 |
|------|------|
| **Windows 10 / 11 (x64)** | 必需（依赖 Win32 控制台 API） |
| **Visual Studio 2022** | 必需。安装时勾选 **「使用 C++ 的桌面开发」** 工作负载（提供 MSVC x64 工具链 `cl.exe`、Windows SDK） |
| **CMake** | 必需。VS 2022 自带（勾选桌面开发即含 CMake/Ninja 组件）；`init_build.bat` 会从 VS 安装目录动态查找 |
| **Git**（可选） | 用于克隆仓库 |
| **VS Code**（可选） | 若用于编辑/IntelliSense，需安装 **C/C++ 扩展** |

> 工具链定位不依赖硬编码路径：脚本通过 `vswhere.exe` 自动查找 VS 安装位置并调用 `vcvarsall.bat x64`。确保命令提示符中能 `where cmake` 找到 CMake。

---

## 快速开始

### 第 1 步：初始化（首次必做）

双击 / 在仓库根目录运行：

```bat
init_build.bat
```

它会依次完成 4 件事：

1. 配置 CMake（生成 `build-msvc\`，VS 2022 x64 生成器）；
2. **编译模块库**（`klogger/kson/kcli/kutil/ktimer/kf` 及 `kmath` 大数模块，产出 `.ifc`/`.lib`）；
3. 生成 `test\build.bat`；
4. 在 `study\` 下所有含 `.cpp` 的目录生成各自独立的 `build.bat`。

> init_build **只**负责配置 + 编译模块库 + 生成脚本，**不会**编译任何 `.cpp`，也不会改动 `Release\`。

### 第 2 步：构建并运行测试

```bat
test\build.bat
```

构建全部测试程序（依赖模块自动编译），产物输出到 `test\Release\`。运行示例：

```bat
test\Release\dbgkmath.exe
test\Release\dbgkson.exe
test\Release\dbgkcli.exe
```

> 模块库被修改后，测试程序会自动重新编译；如需显式全部重建模块 + 测试，可运行 `init_build.bat --build`。

### 第 3 步：构建「学习」程序

```bat
study\algorithm\sorting\legacy\build.bat
```

脚本会列出该目录下所有 `.cpp`，输入编号（如 `1 3`）选择部分或直接回车编译全部，产物输出到同目录 `Release\`。

---

## init_build.bat 参数

| 参数 | 作用 |
|------|------|
| （无参数） | 默认：配置 CMake + 编译模块库 + 生成 build.bat |
| `--config` | 仅配置 CMake（`build-msvc`） |
| `--build` | 显式构建**全部**模块库 + 测试程序（对应 `cmake --build build-msvc`） |
| `--setup` | 仅设置 MSVC 环境（供老式手动 `cl` 使用） |
| `--ide` | 仅生成 `build-ide\compile_commands.json`（供 VS Code IntelliSense，不编译、不改动模块/Release） |

### 取消 / 清理

```bat
cancel_init.bat
```

删除构建目录（`build\ build-msvc\ build20\ build-ide\ build-null\`）与所有 `Release\`、生成的 `build.bat` 等构建产物。

---

## 项目结构

```
kForge/
├── modules/cpp/            # 库源码（C++20 模块，.ixx）
│   ├── klogger.ixx         # 日志 + 颜色常量
│   ├── kson.ixx            # KSON 配置解析 + 文件读取
│   ├── kcli.ixx            # 命令行交互（kout/kin/KBegin/kout）
│   ├── kutil.ixx           # 可视化：迷宫/柱状图打印、控制台适配
│   ├── ktimer.ixx          # 计时器
│   ├── kf.ixx              # 伞形模块：re-export 上面全部
│   ├── kmath/kbignum/      # 大数分层模块
│   │   ├── kbigint.ixx     # 大整数
│   │   ├── kbigdec.ixx     # 大小数
│   │   ├── kbigfrc.ixx     # 分数
│   │   ├── kbigcpx.ixx     # 复数
│   │   ├── kbignum.ixx     # 大数伞形
│   │   └── kmath.ixx       # 大数顶层伞形
│   └── reference.md        # API 参考
├── test/                   # 系统测试程序（import kf / kbignum）
│   └── dbgxxx.cpp          # 各模块测试（dbgkmath/dbgkson/dbgkcli/...）
├── study/                  # 算法与 C++ 特性学习代码（含 legacy/）
├── config/                 # 配置文件
│   ├── global.kson         # 全局配置（GLOBAL）
│   ├── algorithm/          # 算法测试数据
│   └── test/               # 测试数据（cfg.kson 等）
├── CMakeLists.txt          # 模块库 + 测试构建定义
├── init_build.bat          # 初始化脚本
└── cancel_init.bat         # 清理脚本
```

---

## 用模块编写程序

### 最小程序（import kf）

```cpp
import kf;

int main()
{
    kson doc = ReadKsonFile("config/test/cfg.kson");
    KBegin(doc["dbgKCLI"]["meta"]);          // 用配置 meta 初始化标题框

    kout  << "朴素输出" << 42 << std::endl;   // 天蓝色
    koutW << "警告" << std::endl;             // 淡黄
    koutE << "错误 code=" << 404 << std::endl; // 橙色
    kout  << "{green}绿字{/}{bold}{blue}粗蓝{/}" << std::endl; // 颜色标签

    KEnd();                                  // 暂停 + 退出
}
```

> `import kf` 一次性引入 `klogger/kson/kcli/kutil/ktimer`。若要用大数，额外 `import kbignum`。

### 大数运算（import kbignum）

```cpp
import kf;
import kbignum;
#include <iostream>

int main()
{
    KBegin({"大数示例", "BigDec 运算", "author", "2026-09-06"});

    BigDec a("123456789012345678901234567890");  // 超过 long long 也 OK
    BigDec b("-0.0001");
    kout << a * b << std::endl;                  // 自动输出十进制
    kout << Pow(BigDec(2), BigDec(10)) << std::endl; // 1024

    KEnd();
}
```

### 读取配置（kson）

```kson
# config/global.kson
"global":
{
    "meta": { "name": "global", "author": "ByeByeBite" },
    "data":
    {
        "MazePrintSleep": 10,   # 迷宫动画停顿(ms)
        "ArrPrintSleep" : 10,    # 排序动画停顿(ms)
        "BarMax"         : 100   # 排序条最大长度
    }
}
```

程序里通过 `KBegin` 后即可用全局对象 `GLOBAL` 读取（`KBegin` 自动加载 `config/global.kson` 的 `data` 节点）：

```cpp
import kf;
long long pause = GLOBAL["ArrPrintSleep"].Int();   // 10
```

配置文件统一放在根目录 `config/` 下；测试数据用 `config/test/cfg.kson`。路径以项目根（`.kson` 读取时向上查找 `CMakeLists.txt` 锚定）为准，与 exe 的运行目录无关。

---

## 测试程序清单

| 文件 | 覆盖模块 | 说明 |
|------|----------|------|
| `test/dbgklogger.cpp` | klogger | 日志宏、错误码、颜色常量 |
| `test/dbgkson.cpp` | kson | 解析、取值、路径访问、BigNum/科学计数法 |
| `test/dbgkcli.cpp` | kcli + kutil | kout/kin、颜色标签、KOptions、迷宫打印 |
| `test/dbgktimer.cpp` | ktimer | 计时器生命周期与打印 |
| `test/dbgkfio.cpp` | kson(文件读取) | 文件读取与 Fatal 测试 |
| `test/dbgkmath.cpp` | kbignum | 大数出入、四类、inf/nan、Root、NTT（配置驱动） |
| `test/dbgkutility.cpp` | kutil | 工具/可视化 |

---

## FAQ

**Q：为什么运行 `init_build.bat` 报 CMake/找不到？**
A：确认 PATH 中有 `cmake`，且已安装 VS 2022 的「使用 C++ 的桌面开发」组件。

**Q：VS Code 里 module 报错 / IntelliSense 不识别 `import`？**
A：先运行 `init_build.bat --ide` 生成 `build-ide\compile_commands.json`，并确认 `.vscode/c_cpp_properties.json` 的 `compileCommands` 指向它后重启 VS Code。

**Q：改完模块库不生效？**
A：模块库改动需重新编译。直接运行 `test\build.bat` 或 `init_build.bat --build` 会顺带重编模块。

---
## 许可

本项目为个人学习项目，持续迭代中。