# KForge API Reference

> **最后更新**: 2026-08-13 | **标准**: C++17 | **编译器**: MSVC 19.44 (x64)
> **头文件**: `base/KF.hpp`（唯一入口，`#include` 即可使用全部功能）
---

## 目录

- [命名空间结构](#命名空间结构)
- [KLOGGER 日志模块](#klogger-日志模块)
- [KSON 解析模块](#kson-解析模块)
- [KFIO 文件读写模块](#kfio-文件读写模块)
- [KCLI 命令行交互模块](#kcli-命令行交互模块)
- [KTIMER 计时器模块](#ktimer-计时器模块)
- [KMATH 大数运算模块](#KMATH-大数运算模块)
- [KSON 数据格式](#kson-数据格式)
- [构建系统](#构建系统)
- [错误码速查表](#错误码速查表)
- [快速上手](#快速上手)

---

## 命名空间结构

```
KF
├── KLOGGER      日志、错误码、颜色常量
├── KSON         数据解析、节点树、路径访问
├── KFIO         文件读取
├── KTIMER       计时器管理
├── KMATH      大数运算
├── KUTILITY     内部工具函数(不必了解)
└── KCLI         命令行 UI、链式 I/O
```

头文件底部 `using namespace KF::KLOGGER;` 将错误码、`Color`、`LogLevel`、`Module`、`MakeCode` 引入全局，调用处无需前缀。

---

## KLOGGER 日志模块

**源文件**: `KLOGGER.cpp` | **命名空间**: `KF::KLOGGER`

### Color 颜色常量

VT100 转义序列，`KBegin()` 和 `Log()` 自动启用对应句柄的 VT100 处理。

| 常量 | 转义码 | 用途 |
|------|--------|------|
| `Reset` | `\033[0m` | 重置所有样式 |
| `Red` | `\033[31m` | Fatal / koutF |
| `Green` | `\033[32m` | Info |
| `Yellow` | `\033[33m` | 标准黄色 |
| `Blue` / `Magenta` / `Cyan` | `34m` / `35m` / `36m` | 基本色 |
| `LightYellow` | `\033[93m` | Warning / koutW |
| `Orange` | `\033[38;5;208m` | Error / koutE |
| `SkyBlue` | `\033[38;5;75m` | kout 默认色 / 框线 |
| `Bold` | `\033[1m` | 加粗（可叠加） |

### LogLevel / Module / MakeCode

```cpp
enum class LogLevel : uint32_t { Info=1, Warning=2, Error=3, Fatal=4 };
```

| Module | 值 | Module | 值 |
|--------|----|--------|----|
| `Unknown` | `0x00` | `KSON` | `0x03` |
| `Common` | `0x01` | `KTIMER` | `0x04` |
| `KFIO` | `0x02` | `KCLI` | `0x05` |
| `KMATH` | `0x06` | | |

```cpp
constexpr Code MakeCode(uint32_t module, LogLevel level, uint32_t type, uint32_t id) noexcept;
```

错误码格式 `0x[aa][b][cc][ddd]`：`[aa]`=模块(bit24-31) `[b]`=等级(bit20-23) `[cc]`=类型(bit12-19) `[ddd]`=序号(bit0-11)。

> 错误码 `extern` 声明在 `KF.hpp`，定义在 `KLOGGER.cpp`（通过 `MakeCode` 组装），改码时只需重编译 `KLOGGER.cpp`。

### KLOG_* 宏

自动捕获 `__FILE__` / `__LINE__` / `__FUNCTION__`：

```cpp
KLOG_INFO(code, extra)      // Info 级别，程序继续
KLOG_WARNING(code, extra)   // Warning 级别，程序继续
KLOG_ERROR(code, extra)     // Error 级别，程序继续
KLOG_FATAL(code, extra)     // Fatal 级别，system("pause") + exit(EXIT_FAILURE)
```

输出到 `stderr`，格式：`[ERROR]  Code: 0x03301001 Msg: ... | extra | at file:line (func)`

### Table 码表

`extern std::unordered_map<Code, std::string_view> Table;` 定义在 `KLOGGER.cpp`，键为错误码，值为说明文本。新增错误码时必须同步添加条目。

---

## KSON 解析模块

**源文件**: `KSON.cpp` | **命名空间**: `KF::KSON`

### NodeType 枚举

```cpp
enum class NodeType { kInt, kDec, kBig, kStr, kBool, kArr, kObj, kNull };
```

### Node 类

`std::variant` 存储的 KSON 数据树节点。

| 构造 | 类型 |
|------|------|
| `Node()` | kNull |
| `Node(bool)` | kBool |
| `Node(long long)` | kInt |
| `Node(double)` | kDec |
| `Node(BigNum)` | kBig |
| `Node(string)` | kStr |
| `Node(vector<Node>)` | kArr |
| `Node(vector<pair<string,Node>>)` | kObj |

类型判断：`IsNull()` `IsBool()` `IsInt()` `IsDec()` `IsBig()` `IsNumber()` `IsString()` `IsArray()` `IsObject()`

取值（类型不匹配时 `KLOG_ERROR(KSON_TYPE_MISMATCH)`）：

| 方法 | 返回 | 备注 |
|------|------|------|
| `AsBool()` | `bool` | |
| `AsInt()` | `long long` | |
| `AsDec()` | `double` | int 自动转 double |
| `Big()` | `const BigNum&` | 大数引用 |
| `AsStr()` | `string_view` | |
| `AsArr()` | `const vector<Node>&` | |
| `AsObj()` | `const vector<pair<string,Node>>&` | |

| 方法 | 说明 |
|------|------|
| `size()` | 数组/对象元素个数，标量返回 0 |
| `find(key)` | 对象按键查找（区分大小写），返回 `const Node*`，未找到 `nullptr` |
| `at(index)` | 数组按下标查找，越界 `nullptr` |

### PathSeg

路径片段：`PathSeg("key")` 键访问，`PathSeg(3)` 下标访问。

### NodePtr（别名 `kson`）

KSON 树智能指针，持有根节点和访问路径，支持链式 `[]` 延迟解析。

```cpp
kson doc = read(Preprocess(KFIO::ReadFileRaw("cfg.txt")));
auto val = doc["test"]["main"]["Array_Test"][0];
```

| 方法 | 返回 | 路径不存在时 |
|------|------|-------------|
| `TryResolve()` | `const Node*` | `nullptr` |
| `Resolve()` | `const Node*` | KLOG_FATAL(UNKNOWN) |
| `Str()` / `Int()` / `Dec()` / `Big()` / `Bool()` / `Size()` | 对应类型 | Fatal |
| `Exists()` | `bool` | `false` |
| `Auto()` | `string` | `"null"` |

`Auto()` 按类型自动转换：kNull→`null` kBool→`true/false` kInt→整数 kDec→浮点(去尾零,15位) kBig→大数字符串 kStr→原文 kArr→`[e1,...]` kObj→`{"key":val,...}`

静态工厂：`Parse(text)` 从字符串解析，`ParseFile(filepath)` 从文件解析。

### Parser（内部）

递归下降解析器：`ParseStr`(字符串,处理转义) `ParseNum`(整数/小数/科学计数法/大数,支持B后缀,自动转BigNum) `ParseVal`(分派) `ParseArr`(数组,支持尾随逗号) `ParseObj`(对象,支持尾随逗号/重复键后覆盖) `ParseImplicitObj`(隐式顶层对象)

**跨行字符串缩进**：`ParseStr` 遇到换行后跳过行首的缩进空格/制表符，因此多行字符串的值不含行首缩进（源文件里保留缩进，读取时自动去除）。字符串开头的空格不受影响，显式 `\t` 转义保留。

```cpp
// cfg.kson 中：
"color": "{red}红{/}
          {green}绿{/}"
// 解析后值为："{red}红{/}\n{green}绿{/}"（行首缩进已去除）
```

### 自由函数

```cpp
string Preprocess(string_view raw);     // 去注释(#)、去空白、保留字符串内容
kson   read(string_view processed);      // 解析预处理后文本
kson   ReadKsonFile(std::string filepath);    // 一站式：读文件→预处理→解析
> **配置文件路径**：kson 文件统一放在根目录 `config/` 下。测试数据使用 `config/test/cfg.kson`，通用配置使用 `config/config.kson`。路径相对于项目根目录（运行 exe 时 CWD 须为项目根）。
```

---

## KFIO 文件读写模块

**源文件**: `KFIO.cpp` | **命名空间**: `KF::KFIO`

```cpp
std::string ReadFileRaw(std::string_view filepath);
```

二进制模式读取文件全部内容。失败时 Fatal 并附带绝对路径 + errno。

**读取后处理**：自动过滤所有制表符（`\t`），方便直接用于终端输出对齐。

> **路径约定**：相对路径自动拼接项目根目录（向上查找 `base\KF.hpp` 定位）。kson 文件统一放根目录 `config/` 下。

| 错误码 | 级别 | 说明 |
|--------|------|------|
| `KFIO_FILE_OPEN_FAIL` | Fatal | 文件打开失败 |
| `KFIO_FILE_READ_FAIL` | Fatal | 文件读取失败 |

---

## KCLI 命令行交互模块

**源文件**: `KCLI.cpp` | **命名空间**: `KF::KCLI`

### Kout / Kin 链式 I/O

`Kout` 每次 `<<` 自动套用默认颜色，遇 `endl`/`flush` 先输出 `Reset`。对 `std::string` 和 `const char*` 自动解析颜色标签与转义序列。`Kin` 以 `kin >> a >> b >> c;` 为一组进行**整组输入与校验**：按空格/换行分隔读入等量值，自动按变量类型转换；任一非法或个数不符则丢弃整组、给出醒目提示并整组重试，直到全部合法。不同类型严格校验：整数必须为纯整数（带小数点如 `3.14` 判非法），`bool` 支持 `true/false/yes/1/0` 等（大小写不敏感）。

| 全局对象 | 默认颜色 | 用途 |
|----------|----------|------|
| `kout` | SkyBlue | 普通输出 |
| `koutW` | LightYellow | 警告输出 |
| `koutE` | Orange | 错误输出 |
| `koutF` | Red | 致命输出 |
| `kin` | — | 链式输入 |

```cpp
kout  << "普通信息" << 42 << std::endl;       // 天蓝色
koutE << Color::Red << "临时换色" << std::endl; // 临时红色
int age; string name; kin >> age >> name;       // 整组输入，空格/换行分隔，任一非法则整组重试
```

#### 颜色标签 `{tag}`

`kout` 输出字符串时自动解析 `{}` 内的颜色/样式标签，未识别标签原样保留：

| 分类 | 标签 |
|------|------|
| 前景色 | `{red}` `{green}` `{blue}` `{yellow}` `{skyblue}` `{orange}` `{magenta}` `{cyan}` `{lightyellow}` |
| 样式 | `{bold}` `{dim}` `{underline}` `{blink}` |
| 重置 | `{/}` 全部重置 · `{/bold}` `{/underline}` `{/dim}` `{/blink}` 关闭单个 |

```cpp
kout << "{red}红字{/} 普通 {bold}{blue}粗蓝字{/}" << std::endl;
koutW << "{bold}{yellow}警告{/} 信息" << std::endl;   // 叠加样式
```

标签可嵌套叠加，`{red}{bold}粗红{/}` 输出后 `{/}` 一次全部重置。

#### 转义序列

`kout` 同时解析以下反斜杠转义（未知转义如 `\x` 原样保留）：

| 转义 | 含义 | 转义 | 含义 |
|------|------|------|------|
| `\n` | 换行 | `\t` | 制表符（被过滤） |
| `\r` | 回车 | `\"` | 双引号 |
| `\b` | 退格 | `\\` | 反斜杠 |

> **制表符统一过滤**：真实制表符被过滤，因此从文件读取的多行文本在终端不会出现缩进。

### CLI 功能函数

| 函数 | 说明 |
|------|------|
| `KBegin(title, desc)` | 初始化 CLI：启用 VT100、UTF-8、设置标题、打印标题框（2 参） |
| `KBegin(title, desc, author)` | 同上，作者以亮黄显示（3 参） |
| `KBegin(title, desc, author, date)` | 同上，作者与日期（日期）亮黄显示（4 参） |
| `KBegin(kson file)` | 从 KSON 文件读取 `meta` 字段（含 `author`/`date`）初始化 |
| `KOptions(kson menu)` | 显示选项菜单，返回选中索引(0-based)。入参 `"name"`/`"options":[...]` |
| `kpause()` | 显示"按任意键继续..."并等待 |
| `KEnd()` | `kpause()` + `exit(0)` |

> **缺键兜底**：`KBegin(kson)` 读取 `meta` 时若 `cmdtitle`/`title`/`description`/`author`/`date` 缺失，自动用 `KBEGIN_UNKNOWN`（`"Unknown"`）补充，不报错。

---

## KTIMER 计时器模块

**源文件**: `KTIMER.cpp` | **命名空间**: `KF::KTIMER`

```cpp
enum class TimeUnit { ns, us, ms, s };
enum class TimerState { Running, Paused };
```

| 函数 | 返回 | 说明 |
|------|------|------|
| `AddTimer(name, unit)` | `bool` | 新建计时器（创建即开始），同名覆盖返回 false |
| `PauseTimer(name)` | `bool` | 暂停（累计已运行时间） |
| `StartTimer(name)` | `bool` | 恢复已暂停的计时器 |
| `DeleteTimer(name)` | `bool` | 删除计时器 |
| `GetTimer(name)` | `double` | 获取累计时间（不存在返回 -1.0） |
| `PrintTimer(name)` | `void` | 打印单个计时器（格式化框） |
| `PrintAllTimers()` | `void` | 打印所有计时器（格式化表格，按名称排序） |

---

## KMATH 大数运算模块

**源文件**: `KMATH.cpp` | **命名空间**: `KF::KMATH`

### 存储模型

base = 10⁹，每个 `limb`（`uint32_t`）存储 9 位十进制数字。选择 10⁹ 而非 2³² 是为了简化 `ToStr` 的十进制输出。

| 成员 | 类型 | 说明 |
|------|------|------|
| `limbs` | `vector<limb>` | 低位在前，`limbs[0]` 是最低 9 位 |
| `isneg` | `bool` | 负数标志 |
| `scale` | `size_t` | 小数位数 |
| `state` | `State` | 特殊状态：`Normal` / `Inf` / `NegInf` / `Nan` |

类型别名：`using limb = uint32_t; using dlimb = uint64_t;`

### 特殊状态（inf / -inf / nan）

遵循 IEEE-754 语义，用于表示无穷大和非数：

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `Normal` | `State::Normal` | 普通数值（默认） |
| `Inf` | `State::Inf` | 正无穷大 (`+inf`) |
| `NegInf` | `State::NegInf` | 负无穷大 (`-inf`) |
| `Nan` | `State::Nan` | 非数 (`nan`) |

**辅助方法**：

- `IsInf()` — 判断是否为 ±inf
- `IsNan()` — 判断是否为 nan
- `IsNormal()` — 判断是否为普通数值

**构造**：

| 输入 | 结果 |
|------|------|
| `"inf"` / `"+inf"` / `"Inf"` / `"INF"` | `State::Inf` |
| `"-inf"` / `"-INF"` | `State::NegInf` |
| `"nan"` / `"NaN"` / `"NAN"` | `State::Nan` |
| `BigNum(BigNum::State::Inf)` | `State::Inf` |
| `BigNum(BigNum::State::NegInf)` | `State::NegInf` |
| `BigNum(BigNum::State::Nan)` | `State::Nan` |

字符串构造时大小写不敏感，容忍首尾空白（如 `"  -inf  "` → `NegInf`）。

**比较规则**（IEEE-754）：
- NaN 与任何值（包括自身）比较均返回 `false`（`==` 和 `<` 均返回 `false`，`!=` 返回 `true`）
- `-inf < 一切普通数值 < +inf`
- `inf == inf`，`-inf == -inf`，其余特殊状态间不等

**算术传播规则**：

| 运算 | 规则 |
|------|------|
| `+` | 任一 NaN → NaN；`inf + (-inf)` → NaN；其余取无穷 |
| `-` | 任一 NaN → NaN；`inf - inf` → NaN；其余按 `a + (-b)` 处理 |
| `*` | 任一 NaN → NaN；`inf × 0` → NaN；`±inf × ±非零` → ±inf |
| `/` | 任一 NaN → NaN；`inf / inf` → NaN；`±inf / 有限` → ±inf；`有限 / ±inf` → 0；`非零/0` → 报错 ±inf；`0/0` → 报错 NaN |
| `%` | NaN/±inf 参与取模 → NaN；除零先报错再返回 NaN；余数符号跟随被除数 |
| 除零 | 报 `KMATH_DIVBYZERO` 错误，`0/0` → NaN，`非零/0` → ±inf |
| `Pow` | 见下方「幂运算规则」 |

### 幂运算规则

`Pow(a, b)` 为自由函数（不在类内），`a^b`，按 IEEE-754 惯例处理特殊状态：

| 条件 | 结果 |
|------|------|
| `a` 或 `b` 为 `nan` | `|a| == 1` → `1`，否则 → `nan` |
| `a` 为 `±inf`，`b == 0` | `1` |
| `a` 为 `±inf`，`b` 为 `±inf` | `+inf` 指数 → `inf`；`-inf` 指数 → `0` |
| `a` 为 `±inf`，`b < 0` | `0` |
| `+inf` 且 `b > 0` | `inf` |
| `-inf` 且 `b > 0` 整数 | 奇数次 → `-inf`，偶数次 → `+inf` |
| `-inf` 且 `b > 0` 非整数 | `nan` |
| `b` 为 `±inf`，底数有限 | `|a| == 1` → `1`；`a^+inf`：`|a|>1` → `inf`、`|a|<1` → `0`；`a^-inf` 相反 |
| `a == 0`，`b == 0` | `1` |
| `a == 0`，`b < 0` | `inf` |
| `a == 0`，`b > 0` | `0` |
| `a < 0` 且 `b` 非整数 | `nan`（实数域无意义） |
| `b < 0`（其余） | `1 / a^|b|`（保留 9 位小数） |
| `b` 为整数（其余） | 二进制快速幂，精确 |
| `b` 为分数（其余） | 约分后经 `Root` 开根：`a^(p/q) = (a^(1/q))^p`，保留 9 位小数 |

### Normalize 字符串合法化

```cpp
std::string Normalize(const std::string& str);
```

规整为 `[+|-]digits[.digits]` 格式：提取有效字符、去前导零、去小数点后尾随零、多余小数点取分隔、多余正负号异或、全零返回 `"0"`。

| 输入 | 输出 |
|------|------|
| `000012340.3221000000` | `+12340.3221` |
| `-.123.123.` | `-0.123123` |
| `-0` | `0` |
| `dw1-abca00432.432100` | `+100432.4321` |

### ToBig / ToStr

```cpp
static BigNum ToBig(const std::string& str);  // Normalize 后字符串 → BigNum
std::string ToStr() const;                     // BigNum → 字符串
```

**ToBig** 流程：取符号 → 定位小数点算 scale → 提取纯数字 → 从右往左每九位截取一个 limb（`substr` 批量截取，O(n)）→ 去高位零块 → 零值强制 `isneg=false`。

**ToStr** 流程：零值返回 `"0"` → 最高位 limb 不补零，其余补零到 9 位 → 按 scale 插入小数点 → 负数加 `-`。

### 构造函数

| 构造 | 说明 |
|------|------|
| `BigNum()` | 默认构造，零值 |
| `BigNum(const string& str)` | 字符串构造（inf/-inf/nan 大小写不敏感；否则 Normalize → ToBig） |
| `BigNum(算术类型)` 模板 | 任意算术类型（int/long/unsigned/double/float 等）构造，自动正确处理负数/小数（经字符串转换） |
| `BigNum(State s)` | 特殊状态构造（`State::Inf` / `NegInf` / `Nan`） |

### 运算接口

| 分类 | 函数 | 状态 |
|------|------|------|
| 绝对值运算 | `AbsAdd` `AbsSub` `AbsCmp` | 已实现 |
| 绝对值运算 | `AbsMul` `AbsDiv` | 已实现（`AbsDiv` 自动检测：a、b 均为整数且能整除 → 整数，否则保留 9 位小数） |
| 绝对值运算 | `AbsMod` | 已实现（对齐小数位后做整数取余，余数非负） |
| 用户运算符 | `operator+ - * / %` | 已实现（含 inf/nan 状态传播） |
| 用户运算符 | `Pow` | 已实现（整数指数快速幂，负指数 a^{-n}=1/a^n，分数指数经 Root 开根，inf/nan 规则） |
| 开根 | `Root` | 已实现（返回 `BigCpx`：n 次方根 a^(1/n)，保留 9 位小数；负指数取倒数；负数开偶次方 → 纯虚数，如 `√-4 = "2i"`） |
| 类型查询 | `type()` | 已实现（返回 `"nan"`/`"inf"`/`"-inf"`/`"int"`/`"dec"`） |
| 算术类型混合 | `operator+ - * /` 模板 | 已实现（BigNum 可与所有算术类型 int/long/float/double 等的右值和变量互算，含反向 `5 + bn`、`2.5 * bn`） |
| 比较运算符 | `operator< == != <= > >=` | 已实现（IEEE-754：NaN 参与比较恒为 false） |
| 输出 | `friend operator<<` | 调用 `ToStr`（inf → `"inf"`，nan → `"nan"`） |

**除法精度**：`AbsDivSchool` 保留 `keep` 位小数（默认 9），逐块（每块 9 位）二分试商，在 `[0, BASE-1]` 内二分查找最大商 d 使 `d*B <= 当前余数`，与手算长除法一致。

> **设计约定**：自由函数（`AbsCmp` 等）声明放在 `BigNum` 类定义之前，需前向声明 `class BigNum;`。类内调用时省略 `KMATH::` 前缀。`AbsAdd` 中使用 `(std::max)` 避免 Windows max 宏冲突。

```cpp
#include "KF.hpp"
using namespace KMATH;
BigNum a("123456789012345678901234567890");
BigNum b("-0.0001");
kout << a << "\n" << b << "\n";  // 123456789012345678901234567890 / -0.0001
```

### 四大数类（BigInt / BigDec / BigFrc / BigCpx）

大数模块由共享基类 `BigNum` 派生出的四类组成，按等级自动提升运算：

| 类型 | 全称 | 说明 | 类型等级 |
|------|------|------|----------|
| `BigInt` | 大整数 | `scale` 恒为 0 的纯整数，构造自动取整截断小数 | 1 |
| `BigDec` | 大小数 | 整数 + `scale` 位小数，最通用，绝大多数用法 | 2 |
| `BigFrc<N,D>` | 分数 | 分子 N、分母 D，分母恒归一化为正，默认 `BigFrc<BigInt,BigInt>` | 3 |
| `BigCpx<R,I>` | 复数 | 实部 R、虚部 I，默认 `BigCpx<BigDec,BigDec>` | 4 |

**跨类型运算自动提升**：如 `BigInt + BigDec = BigDec`、`BigDec + BigFrc = BigFrc`、`BigFrc + BigCpx = BigCpx`（低等级自动转为高等级参与运算）。

存储模型均为 `base = 10⁹` 小端 `vector<limb>`（`limb = uint32_t`），支持 `inf` / `-inf` / `nan` 特殊状态（IEEE-754 语义）。

#### 构造

| 构造 | 说明 | 示例 |
|------|------|------|
| 默认构造 | 零值 | `BigDec x;  // 0` |
| 字符串构造 | 十进制串，大小写不敏感解析 `inf/-inf/nan`，容忍首尾空白 | `BigDec("3.14")` `BigInt("123456789012345678901234567890")` |
| 算术类型模板 | int/long/unsigned/double/float 等自动构造（内部经字符串转换） | `BigDec(42)` `BigInt(2.7)  // 2` `BigDec(0.5f)` |
| 特殊状态构造 | `State::Inf` / `NegInf` / `Nan` | `BigDec(BigDec::State::Inf) // "inf"` |
| `BigInt(BigNum)` | 十进制 → 取整截断小数 | `BigInt(BigDec("3.9")) // 3` |
| `BigDec(BigInt)` | 整数 → 小数（scale=0） | `BigDec(BigInt(5)) // 5` |
| `BigFrc(num, den)` | 分子/分母；分母为 0 时置 0；分母为负自动归一化为正 | `BigFrc<>(1, 2) // "0.5"` |
| `BigFrc(BigDec)` | 小数 → 精确分数 | `BigFrc<>(BigDec("0.25")) // 1/4` |
| `BigCpx(re, im)` | 实部 + 虚部 | `BigCpx<>(1, 2) // "1+2i"` |
| CTAD 自动推断 | 标量组件直接推断 int/double | `BigFrc(1,2)` `BigCpx(1,2)` |

> `BigFrc<>` / `BigCpx<>` 即 `BigFrc<BigInt,BigInt>` / `BigCpx<BigDec,BigDec>`，与旧用法完全兼容。

#### 成员 / 访问

```cpp
BigDec x("123.4500");
x.limbs;      // vector<limb>：base=1e9 小端，limbs[0] 为最低 9 位
x.isneg;      // bool 是否为负
x.scale;      // size_t 小数位数（BigInt 恒为 0）
x.state;      // State：Normal/Inf/NegInf/Nan
x.IsInf();    // 是否 ±inf
x.IsNan();    // 是否 nan
x.IsNormal(); // 是否普通数值
x.type();     // "nan" / "inf" / "-inf" / "int" / "dec"（BigFrc 返回 "frac"，BigCpx 返回 "cpx"）
x.ToStr();    // 十进制字符串
```

#### 基本运算

```cpp
BigDec a("10"), b("4");
a + b;   // 14
a - b;   // 6
a * b;   // 40
a / b;   // 2.5（整除则整数，否则保留 9 位小数）
a % b;   // 2（余数符号随被除数）
-b;      // -10
+a;      // 10
Pow(a, b);   // a^b：整数指数精确快速幂；分数指数经 Root 开根；负指数 1/a^|b|；支持 inf/nan 规则
Root(a, b);  // a^(1/b)，返回 BigCpx；负数开偶次方返回纯虚数，如 Root("-4","2") == "2i"
BigGcd(a, b); // 非负整数最大公约数（欧几里得）
```

**纯虚数 / 复数求值**：`Root` 返回 `BigCpx`，即其实数结果加 `"i"`：

```cpp
BigCpx<> z(3, 4);
z.Conj();      // 3-4i
z.Abs();       // 5（BigDec 模长）
Pow(z, 2);     // -7+24i
Abs(z);        // 5（自由函数，返回 BigDec）
Conj(z);       // 3-4i（自由函数，返回 BigCpx）
```

**与原生算术类型混合**（四类都支持左右双向）：

```cpp
BigDec("5") + 3;   // 8
BigDec("10") / 4;  // 2.5
2.5 * BigDec("4"); // 10
7 / BigDec("2");   // 3.5
int i = 2; double d = 1.5;
BigDec("3") + i;   // 5
d + BigDec("2.5"); // 4
```

#### 比较与输出

```cpp
a == b;  a != b;  a < b;  a <= b;  a > b;  a >= b;
// IEEE-754：NaN 与任何值（包括自身）比较恒为 false
kout << a << std::endl;          // 调用 ToStr；inf → "inf"，nan → "nan"
std::cout << a;                  // ostream operator<< 同样可用
std::cin >> a;                   // istream operator>>（从字符串解析）
```

#### 类型转换

| 转换 | 说明 |
|------|------|
| `x.ToStr()` | 十进制度字符串 |
| `x.ToBigDec()` | 转小数（`BigFrc`/构造自它时常用），整除则整数否则 9 位小数 |
| `ToBigDec(x)`（自由函数） | 任意数学类型 → `BigDec`，组件归一化专用 |
| `BigFromDec<R>::from(d)` | `BigDec` → 指定组件类型 R（内部模板） |
| `explicit operator long long()` | 范围内原生转换 |
| `big.IsInLongLongRange()` | 是否在 long long 范围 |
| `big.IsInDoubleRange()` | 是否在 double 范围 |

#### 组件嵌套（BigFrc / BigCpx 的高级用法）

N/D（或 R/I）可为四类数学类型**任意嵌套**，也可为原生算术类型（CTAD 自动推断）：

```cpp
using F = BigFrc<BigInt, BigInt>;
BigCpx<F, F> z(BigFrc<BigInt,BigInt>(1,1), BigFrc<BigInt,BigInt>(2,1));
(z * z).ToStr();            // "-3+4i"

// 分数以分数为分子：(1/2) / 4 = 0.125
BigFrc<F, BigInt> q(BigFrc<BigInt,BigInt>(1,2), BigInt(4));
q.ToBigDec().ToStr();       // "0.125"

// 复数实部为小数、虚部为分数：z = 3 + 0.5i
BigCpx<BigDec, F> m(BigDec("3"), BigFrc<BigInt,BigInt>(1,2));
(m * m).ToStr();            // "8.75+3i"

// 标量组件（CTAD 自动推断 int/double）
BigFrc(1, 2).ToBigDec().ToStr();  // "0.5"
BigCpx(1, 2).ToStr();             // "1+2i"
BigCpx(1, 2).Conj().ToStr();      // "1-2i"
```

#### 随机数生成

```cpp
BigInt  RandBigInt({iMin,iMax}, sign); // 随机整数：{位数下界,位数上界}；sign=0随机/1全正/2全负
BigDec  RandBigDec({iMin,iMax},{dMin,dMax}, sign); // 随机小数：{整数位}、{小数位}、符号
RandBigNum({iMin,iMax},{dMin,dMax},sign); // == RandBigDec 的通用别名
// 示例
BigDec r = RandBigDec({1,5}, {0,3}, 2);   // 5 位以内整数、0~3 位小数、全负数
BigDec z = RandBigDec({0,0},{0,0},0);     // 恒为 0
```

#### 工具 / 规整

```cpp
Normalize("000012340.3221000000"); // "+12340.3221"：去小数点/前后导0，规整为 [+|-]d[.d]
ScaleTo(BigDec("1.5"), 5);         // "1.50000"：对齐小数位（数值不变，要求 newScale >= scale）
```

#### 性能：大数乘法自动 NTT

乘法在 limb 数 ≥ 48（约 432 位十进制）时自动切换到 **NTT（数论变换）+ 中国剩余定理**，复杂度从学校算法 O(n²) → O(n log n)，通过三期素数（998244353 / 1004535809 / 469762049，原根 3，最大变换长度 2²⁶）保证 100,000 位级大数乘法的正确与提速。调用者无需任何改动，`operator*` 自动触发。

```cpp
BigDec A("9"); BigDec B("9");
for(int i = 0; i < 60; i++) { A = A * A; B = B * B; }  // 位宽增长到触发 NTT
BigDec C = A * B;  // 自动走 NTT 路径
```

---

## KSON 数据格式

类 JSON 自定义格式，支持：双引号字符串、整数(`long long`)、小数(`double`)、大数(`BigNum`)、布尔、`null`、数组、对象、`#`行注释、尾随逗号、隐式顶层对象（无需外层`{}`）、转义字符(`\n \t \r \b \" \\`)、重复键后覆盖前、跨行字符串（自动去除行首缩进）。

**大数支持**：
- 科学计数法（如 `1.23e50`）自动解析为 `BigNum`
- 数字后加 `B` 后缀（如 `123456789B`）强制存储为 `BigNum`
- 整数超出 `long long` 范围（`9223372036854775807`）自动转为 `BigNum`
- 关键字 `inf` / `-inf` / `nan`（大小写不敏感，如 `Inf` `NAN`）解析为 `BigNum` 特殊状态
- 引号字符串 `"inf"` `"-inf"` `"nan"`（大小写不敏感）同样自动转为 `BigNum` 特殊状态
- 通过 `Big()` 方法获取 `BigNum` 引用；`Auto()` 可统一转字符串（含特殊状态）

精度限制：整数超出 `long long` 触发 `KSON_PARSE_NUMOR`，科学计数法指数过大触发 `KSON_PARSE_BIG_EXP`。

### KSON 文件实例

以 `config/test/cfg.kson` 为例，每个模块一个对象，均含 `meta`（供 `KBegin` 使用）：

```kson
# KSON 全功能测试配置文件
dbgKSON:
{
    "meta": { "cmdtitle": "dbgKSON", "title": "KSON 模块测试",
              "description": "KSON 解析模块全功能测试",
              "author": "Git-1145", "date": "2026-08-13" },

    "intro": { "integer": 42, "decimal": 3.14, "flag": true, "nothing": null },

    "types":
    {
        "int_value": 42,
        "dec_value": 3.14,
        "str_value": "hello world",
        "bool_true": true,
        "bool_false": false,
        "null_value": null,
        "int_array": [1, 2, 3, 4, 5,],
    },

    "escapes":
    {
        "newline": "line1\nline2",
        "tab": "col1\tcol2",
        "quote": "say \"hi\"",
        "backslash": "path\\to\\file",
        "color": "{red}红{/}
                  {green}绿{/}
                  {blue}蓝{/}",
    },

    "kson_bignum":
    {
        "int64_max": 9223372036854775807,
        "overflow_big": 9223372036854775808,      # 自动转 BigNum
        "sci_big": 1.23e50,                       # 科学计数法 → BigNum
        "big_suffix": 123456789B,                 # B 后缀强制 BigNum
    },
}
```

访问示例：

```cpp
kson doc  = ReadKsonFile("config/test/cfg.kson");
kson meta = doc["dbgKSON"]["meta"];
KBegin(meta);                                   // 用 meta 初始化标题框
auto color = doc["dbgKSON"]["escapes"]["color"].Str();
kout << color << std::endl;                     // 输出带颜色标签的多行文本
```

---

## 构建系统

### 一键构建

| 脚本/文件 | 位置 | 用途 |
|-----------|------|------|
| `init_build.bat` | 根目录 | 预编译 `base/KF.lib` + 在 `study/` 下含 .cpp 的目录生成 `build.bat` |
| `build.bat` | study/ 子目录 | 由 init_build 生成，美观 CLI 选择文件编译 |
| `cancel_init.bat` | 根目录 | 清理所有构建产物（KF.lib、obj、Release/、生成的 bat） |
| `CMakeLists.txt` | 根目录 | 可选，供 IDE / CI 使用（实际编译用 build.bat） |

### init_build.bat

运行 `init_build.bat` 后：
1. 设置 MSVC 环境
2. 增量预编译 `base/*.cpp` → `base/obj/*.obj` → `base/KF.lib`
3. 扫描 `study/` 下所有含 .cpp 的子目录
4. 每个目录生成 `build.bat`

### build.bat

每个脚本自包含，直接调用 `cl.exe` + `link.exe`，不依赖 CMake：

- **build.bat**：显示文件列表，输入编号选择（如 `1 3`），Enter 编译全部。每次运行前保留 `Release/`
- 编译产物（exe）输出到同目录的 `Release/` 文件夹

### 编译要求

- Visual Studio 2022（含 C++ 工具链）
- Windows 10 SDK
- 首次使用运行 `init_build.bat`

### 配置文件结构

```
项目根目录
├── config/
│   ├── config.kson        # 通用配置（KBegin 使用）
│   └── test/
│       └── cfg.kson       # 测试数据配置
├── base/                  # 基础库源码
├── test/                  # 测试程序
└── study/                 # 学习示例
```
---

## 错误码速查表

### 通用 (01) / KFIO (02)

| 常量 | 码值 | 等级 | 说明 |
|------|------|------|------|
| `TEST_INFO` | `0x01101001` | Info | 测试-信息 |
| `TEST_WARN` | `0x01201002` | Warning | 测试-警告 |
| `TEST_ERROR` | `0x01301003` | Error | 测试-错误 |
| `TEST_FATAL` | `0x01401004` | Fatal | 测试-严重错误 |
| `KFIO_FILE_OPEN_FAIL` | `0x02401001` | Fatal | 文件打开失败 |
| `KFIO_FILE_READ_FAIL` | `0x02401002` | Fatal | 文件读取失败 |

### KSON (03)

| 常量 | 码值 | 等级 | 说明 |
|------|------|------|------|
| `KSON_PARSE_STRE` | `0x03301001` | Error | 字符串不以双引号开头 |
| `KSON_PARSE_STR_NOEND` | `0x03301002` | Error | 字符串缺少闭合双引号 |
| `KSON_PARSE_MULPOINT` | `0x03201003` | Warning | 多个小数点 |
| `KSON_PARSE_NUM_UE` | `0x03201004` | Warning | 数字含非法字符 |
| `KSON_PARSE_NUMOR` | `0x03301005` | Error | 数字超出类型范围 |
| `KSON_PARSE_NUM_USTYPE` | `0x03301006` | Error | 不支持的数字类型 |
| `KSON_PARSE_ESCAPE_SPECIAL` | `0x03201007` | Warning | 无效转义序列 |
| `KSON_PARSE_UNFINISHED_ESCAPE` | `0x0340100A` | Fatal | 转义序列不完整 |
| `KSON_PARSE_BIG_EXP` | `0x0330100C` | Error | 科学计数法指数过大 |
| `KSON_PARSE_VAL_END` | `0x033010AA` | Error | 读到末尾仍缺值 |
| `KSON_PARSE_VAL_ERROR` | `0x033010AC` | Error | 值类型无法识别 |
| `KSON_PARSE_ARR_BEGIN` | `0x033010AE` | Error | 期望数组起始 `[` |
| `KSON_PARSE_ARRUE` | `0x033010B0` | Error | 数组中出现非预期字符 |
| `KSON_PARSE_OBJ_BEGIN` | `0x033010B2` | Error | 期望对象起始 `{` |
| `KSON_PARSE_OBJ_KEY_QUOTE` | `0x033010B4` | Error | 对象键未用双引号包裹 |
| `KSON_PARSE_OBJ_SEPERATOR` | `0x033010B6` | Error | 键后缺少冒号 |
| `KSON_PARSE_OBJUE` | `0x033010B8` | Error | 对象中出现非预期字符 |
| `KSON_PARSE_TRAIL` | `0x03202011` | Warning | 结尾有多余字符 |
| `KSON_TYPE_MISMATCH` | `0x03401001` | Fatal | AsXxx 类型不匹配 |

### KTIMER (04) / KCLI (05) / KMATH (06) / 未知 (00)

| 常量 | 码值 | 等级 | 说明 |
|------|------|------|------|
| `KTIMER_NOT_FOUND` | `0x04201001` | Warning | 计时器不存在 |
| `KTIMER_ALREADY_EXISTS` | `0x04201002` | Warning | 计时器已存在 |
| `KTIMER_STATE_ERROR` | `0x04201003` | Warning | 计时器状态不允许此操作 |
| `KCLI_INPUT_INVALID` | `0x05201001` | Warning | 输入解析失败 |
| `KMATH_MULPOINT` | `0x06201002` | Warning | 多余的小数点 |
| `KMATH_INVALIDCHAR` | `0x06201004` | Warning | 数字中含非法字符 |
| `KMATH_DIVBYZERO` | `0x06302003` | Error | 除零：结果为 ±inf（0/0 为 nan） |
| `UNKNOWN` | `0x00400000` | Fatal | 未知错误码 |

---

## 快速上手

### 最小程序

```cpp
#include "base/KF.hpp"
using namespace KCLI;
using namespace KTIMER;

int main()
{
    KBegin("示例程序", "KForge 快速上手", "Git-1145", "2026-08-13");
    KEnd();
}
```

### 综合示例：KSON + KCLI + KFIO + KTIMER

```cpp
#include "base/KF.hpp"
using namespace KSON;
using namespace KCLI;
using namespace KFIO;
using namespace KTIMER;

int main()
{
    // KSON：读取配置文件（含 meta）
    kson doc = ReadKsonFile("config/test/cfg.kson");
    KBegin(doc["dbgKSON"]["meta"]);               // 用 meta 初始化标题框

    // KCLI：彩色输出 + 颜色标签
    kout << "{red}错误{/} {green}成功{/} {bold}{blue}高亮{/}" << std::endl;
    koutE << Color::Orange << "警告消息" << std::endl;

    // KFIO：读取文件（自动过滤制表符）
    std::string raw = ReadFileRaw("config/test/cfg.kson");
    kout << "文件长度: " << raw.size() << std::endl;

    // KTIMER：计时
    AddTimer("work", TimeUnit::ms);
    for (volatile size_t i = 0; i < 100000000; i++) {}  // 模拟工作
    kout << "耗时: " << GetTimer("work") << " ms" << std::endl;

    // KCLI：菜单（dbgKCLI 的 KOption 含 name/options）
    size_t choice = KOptions(doc["dbgKCLI"]["KOption"]);
    kout << "你选择了: " << choice + 1 << std::endl;

    KEnd();
}
```

### 测试文件

| 文件 | 模块 | 覆盖功能 |
|------|------|----------|
| `dbgKSON.cpp` | KSON | 解析、类型判断、取值、路径访问、find/at、Auto、转义、注释、尾随逗号、重复键、BigNum/科学计数法 |
| `dbgKFIO.cpp` | KFIO | ReadFileRaw 读取/验证、Fatal 测试 |
| `dbgKLOGGER.cpp` | KLOGGER | KLOG_* 宏、错误码、MakeCode、Table、枚举、Color |
| `dbgKCLI.cpp` | KCLI | kout/koutW/koutE/koutF 输出、颜色标签、kin 输入、KOptions 菜单 |
| `dbgKTIMER.cpp` | KTIMER | AddTimer/PauseTimer/StartTimer/DeleteTimer/GetTimer/Print* |
| `dbgkmath.cpp` | KMATH | Normalize、ToBig+ToStr 往返、limbs/scale/isneg 验证、边界值、inf/nan 特殊状态、Root 虚数（配置驱动） |

> Fatal 级别测试放在文件最后执行，触发后终止程序。所有测试脚本使用 `SECTION`/`CHECK`/`SHOW` 宏，配合 `kout`/`koutW`/`koutE`/`koutF` 彩色输出。

### 维护指南

**新增错误码**：`KF.hpp` 加 `extern const Code` → `KLOGGER.cpp` 加 `MakeCode` 定义 + `Table` 条目 → 更新本文档。

**新增模块功能**：`KF.hpp` 对应 namespace 加声明 → `.cpp` 中 `namespace KF::XXX` 实现 → 更新本文档。
