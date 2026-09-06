# KForge API 参考（C++20 模块版）

> **更新**: 2026-09-06 | **标准**: C++20 | **编译器**: MSVC 19.44 (x64)
> 库由具名模块（`.ixx`）组成，符号已去命名空间、平铺到全局。使用前先运行 `init_build.bat` 编译模块库。

---

## 模块结构

| 模块 | 文件 | 作用 | 依赖 |
|------|------|------|------|
| `klogger` | `klogger.ixx` | 日志、错误码、颜色常量 | — |
| `kson` | `kson.ixx` | KSON 解析、文件读取 | `klogger` |
| `kutil` | `kutil.ixx` | 可视化（迷宫/柱状图）、控制台适配 | `klogger` `kson` |
| `kcli` | `kcli.ixx` | 命令行交互（kout/kin/KBegin） | `klogger` `kson` `kutil` |
| `ktimer` | `ktimer.ixx` | 计时器 | `klogger` |
| `kf` | `kf.ixx` | 伞形：re-export 上面全部 | 上述全部 |
| `kbignum` | `kmath/kbignum/` | 大数（int/dec/frc/cpx） | `klogger` |

用法：

```cpp
import kf;            // 一次引入 klogger/kson/kcli/kutil/ktimer
import kbignum;       // 大数模块需单独 import
```

---

## klogger — 日志 / 颜色

```cpp
// 颜色常量（const char*，可直接 << 进 ostream）
Reset  Red  Green  Yellow  Blue  Magenta  Cyan
LightYellow  Orange  SkyBlue  Gray  LightGray  Bold
```

```cpp
// 日志（stderr），自动附 文件:行(函数)
Error(code, extra);  Warning(code, extra);
Info(code, extra);   Fatal(code, extra);       // Fatal → pause + exit

KLOG_ERROR(code, extra);   KLOG_WARNING(code, extra);
KLOG_INFO(code, extra);    KLOG_FATAL(code, extra);
```

---

## kson — 配置解析

```cpp
// 一站式读取（会开启控制台彩色）：
kson doc = ReadKsonFile("config/test/cfg.kson");

// 类型判断 / 取值
doc.IsObject()  doc.IsArray()  doc.IsString()  doc.IsInt()
doc.IsDec()     doc.IsBool()   doc.IsNull()    doc.IsNumber()

doc.AsStr()  doc.AsInt()  doc.AsDec()  doc.AsBool()
doc.AsArr()  doc.AsObj()  doc.size()   doc.find("key")   doc.at(0)

// NodePtr(kson) 链式取值，未命中 key 时用 Resolve 触发 Fatal
auto v = doc["dbgKCLI"]["meta"];
v.Str()  v.Int()  v.Dec()  v.Bool()  v.Size()  v.Exists()  v.Auto()

// 解析字符串 / 文件
kson doc2 = read(Preprocess(str));     // 预处理(#注释/空白) → 解析
kson doc3 = kson::Parse(str);          // 直接解析
```

---

## kcli — 命令行交互

```cpp
kout  << "文字" << 42 << std::endl;     // 默认天蓝色
koutW << "警告" << std::endl;           // 淡黄
koutE << "错误" << std::endl;           // 橙
koutF << "致命" << std::endl;           // 红
// 颜色标签 {tag}，e.g. {red}{bold}{blue}{/} 等，参见 README
kout << "{green}成功{/}" << std::endl;

// 链式输入（整组校验，任一非法整组重试）
int a; std::string s; kin >> a >> s;

// 初始化标题框 / 菜单 / 暂停 / 结束
KBegin({"cmdtitle", "标题", "作者", "2026-09-06"});
KBegin(doc["dbg"]["meta"]);                   // 从配置 meta 读取
size_t i = KOptions({"继续", "退出"}, "选择菜单");  // 0-based
KPause();  KEnd();                            // KEnd = pause + exit
```

---

## kutil — 可视化

```cpp
// 迷宫打印（内部含 \033[H/\033[K，全量刷新）
Maze::Print(maze);   // maze: const std::vector<std::vector<MazeCell>>&

// 数组柱状图（# 条，按序号着色）
Arr::Print(ranks, n, barMax, highlight1, highlight2, sortedUntil);

CheckConsoleFit(rows, cols, keepIfFits=false);  // 静默调整窗口/字号
ClearScreen();
RestoreConsoleFont();
```

---

## ktimer — 计时器

```cpp
AddTimer("name", TimeUnit::ms);   // 创建即开始；已存在返回 false
PauseTimer("name");  StartTimer("name");  DeleteTimer("name");
double d = GetTimer("name");       // 累计时间（不存在返回 -1.0）
PrintTimer("name");  PrintAllTimers();
```

`TimeUnit`: `ns | us | ms | s`；`TimerState`: `Running | Paused`。

---

## kbignum — 大数

四种类型：`BigInt`(整数) `BigDec`(小数，最常用) `BigFrc`(分数) `BigCpx`(复数)。存储 base=10⁹ 小端 `vector<limb>`。

```cpp
BigDec a("123456789012345678901234567890");  // 超过 long long 也可
BigDec b("-0.0001");
a + b;  a - b;  a * b;  a / b;  a % b;               // 混合运算 + - * / %
a == b;  a < b;  a <= b;  a > b;  a >= b;            // IEEE-754（NaN 比较恒 false）
Pow(a, b);   Root(a, b);  BigGcd(a, b);              // 幂 / 开根(返回 BigCpx) / 最大公约数
Normalize("00012.30");                              // → "+12.3"
RandBigInt({1,5}, 0);  RandBigDec({1,5},{0,3}, 0);  // 随机大数
ScaleTo(BigDec("1.5"), 5);                          // 对齐小数位

x.ToStr();   x.limbs;  x.isneg;  x.scale;  x.state;
x.IsInf();   x.IsNan(); x.IsNormal(); x.type();
x.Conj();    x.Abs();                                 // 复数
```

可与原生算术类型双向互算：`BigDec("10") / 4`、`2.5 * BigDec("4")`。

> **性能**：乘法 limb 数 ≥ 48 时自动切 NTT（O(n log n)），调用方无感。

---

## KSON 数据格式

类 JSON：字符串、整数、小数、布尔、`null`、数组、对象、`#`行注释、尾随逗号、隐式顶层对象（无需外层 `{}`）、重复键后覆盖。支持 `inf`/`-inf`/`nan`（大小写不敏感）、科学计数法、`B` 后缀、超 `long long` 整数（转 BigNum）。配置文件统一放 `config/`，路径以项目根锚定（向上找 `CMakeLists.txt`）。

---

## 构建

```bat
init_build.bat          # 配 CMake + 编译模块库 + 生成各目录 build.bat
test\build.bat          # 编译全部测试到 test\Release\
init_build.bat --build  # 显式重建模块库 + 全部测试
init_build.bat --ide    # 生成 build-ide\compile_commands.json（VS Code IntelliSense）
```

---

## 如何新建自己的模块

下面以新建一个简单模块 `kstats`（对数字容器求平均值，并借用 `klogger` 的颜色）为例。

### 1. 新建 `.ixx` 文件

放在 `modules/cpp/` 下（或按需放子目录，如大数模块的 `kmath/kbignum/`）。骨架：

```cpp
// modules/cpp/kstats.ixx
module;                    // ① 模块私有区：include 只对"本模块"可见
#include <vector>

export module kstats;      // ② 声明模块名（须与 CMake 目标逻辑名一致）

import klogger;            // ③ 导入其它模块（不对外转发）

export                    // ④ 对外可见的声明
{
    double Average(const std::vector<double>& v);
    const char* ColorBySign(double x);      // 用 klogger 的 Green/Red
}

// ⑤ export 块之外 = 模块内部定义（对调用方隐藏）
double Average(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    double s = 0;
    for (double x : v) s += x;
    return s / static_cast<double>(v.size());
}
const char* ColorBySign(double x)
{
    return x >= 0 ? Green : Red;           // Green/Red 来自 klogger
}
```

要点：
- `module;` 下的 `#include` 不会泄漏给外部；
- `export module 名字;` 后紧跟 `import 依赖`;
- **声明**写在 `export { ... }` 内，**定义**写在块外即可对外可见；
- 被导入的模块对它自身可见，但不会 re-export 给使用方（除非 `export import`，见下）。

### 2. 分层 / 伞形模块（可选）

如需把模块拆成多个子模块再聚合，参考 `kbignum`：

```cpp
// 子模块：export module kstats.math;       ...
// 伞形模块：export module kstats;           （re-export 子模块）
export import kstats.math;
```

### 3. 在 `CMakeLists.txt` 注册

在 `modules/cpp` 的模块区（`kf` 之前或之后）加入：

```cmake
add_library(kstats)
target_compile_features(kstats PUBLIC cxx_std_20)
target_sources(kstats PUBLIC FILE_SET CXX_MODULES FILES ${MOD}/kstats.ixx)
target_link_libraries(kstats PUBLIC klogger)   # 依赖哪些模块就 link 哪些
set_target_properties(kstats PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules/cpp")
```

### 4. 对外提供

两种方式任选其一：

**A. 并入 `kf` 伞形（推荐，所有测试`import kf`即可用）**：在 `kf.ixx` 加一行 `export import kstats;`，并在 CMake 中给 `kf` 目标追加 `kstats`：

```cmake
target_link_libraries(kf PUBLIC klogger kson kcli ktimer kutil kstats)
```

**B. 仅在特定程序使用**：在其目标追加模块，例如给测试 `dbgxxx`：

```cmake
kforge_add_exe(dbgxxx SRCS test/dbgxxx.cpp MODS kf kstats)
```

（用方式 B 时程序内 `import kstats;` 即可。）

### 5. 编译验证

```bat
init_build.bat --build     # 重建模块库 + 测试
```

### 6. 使用示例

```cpp
// test/dbgkstats.cpp
import kf;
import kstats;
#include <iostream>

int main()
{
    std::vector<double> v = {1.0, 2.5, 3.0};
    kout << ColorBySign(Average(v)) << Average(v) << std::endl;  // 绿色 2.16667
    KEnd();
}
```

### 注意事项

- 模块名不能重复；一个 `.ixx` 对应一个目标。若模块间有依赖，用 `target_link_libraries` 声明，CMake 会自动管理编译顺序与 `.ifc` 传递。
- `import kbignum` 是独立的：基础模块（`kf`）**不含**大数，需要时在目标里同时 link `kbignum`（参考 `dbgkmath` 的 `TEST_MODS`）。
- 修改模块库后需重新编译（`test\build.bat` / `init_build.bat --build`）才能生效。