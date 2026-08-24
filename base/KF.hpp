#pragma once
#include<vector>
#include<algorithm>
#include<stack>
#include<cmath>
#include<string>
#include<queue>
#include<random>
#include<cstdlib>
#include<stdexcept>
#include<windows.h>
#include<functional>
#include<iomanip>
#include<memory>
#include<sstream>
#include<limits>
#include<cerrno>
#include<chrono>
#include<fstream>
#include<iostream>
#include<unordered_map>
#include<variant>
#include<cstring>
#include<type_traits>
#include<cctype>
using Code = uint32_t;
/**
 * @file KF.hpp
 * @brief KForge 所有基础模块的声明文件
 * @version 1.1.0
 * @date 2026-08-24
 * @author Git-1145
 * @usage #include "KF.hpp"
 * @usage using namespace xxx; // xxx 为模块名
**/


#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
// 日志宏：自动捕获调用位置（文件名:行号:函数名）
// 用法：KLOG_ERROR(code, "extra")  替代  KLOG::Error(code, "extra")
/// @attention C++17 无法用默认参数捕获 __FILE__/__LINE__，必须用宏

#define KLOG_ERROR(code, extra)     ::KF::KLOGGER::Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra)   ::KF::KLOGGER::Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)      ::KF::KLOGGER::Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)     ::KF::KLOGGER::Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)
namespace KF
{
    using limb = uint32_t; // 基础分块
    using dlimb = uint64_t; // double limb
    using slimb = int32_t; // signed limb
    using sdlimb = int64_t; // signed double limb

    constexpr const char* FILE_META = "meta";
    constexpr const char* FILE_CMDTITLE = "cmdtitle";
    constexpr const char* FILE_TITLE = "title";
    constexpr const char* FILE_DESC = "description";
    constexpr const char* FILE_AUTHOR = "author";
    constexpr const char* FILE_DATE = "date";
    constexpr const char* FILE_OPTION = "options";
    constexpr const char* FILE_OPTNAME = "name";
    constexpr const char* KBEGIN_UNKNOWN = "Unknown"; // KBegin 读取配置缺键时的默认补充值
    namespace KLOGGER
    {
        /// @brief 输出 [ERROR] 级别日志（程序继续运行）
        /// @param code  错误码
        /// @param extra 附加信息
        /// @param file  源文件名（由宏 __FILE__ 自动捕获）
        /// @param line  源行号（由宏 __LINE__ 自动捕获）
        /// @param func  函数名（由宏 __FUNCTION__ 自动捕获）
        void Error(Code code, const std::string& extra, const char* file, int line, const char* func);
        void Warning(Code code, const std::string& extra, const char* file, int line, const char* func);
        void Info(Code code, const std::string& extra, const char* file, int line, const char* func);
        void Fatal(Code code, const std::string& extra, const char* file, int line, const char* func);
        extern std::unordered_map<Code, std::string_view> Table;// 码表

        /////////////////////////////////////////////////////////
        // 错误码编码系统
        /////////////////////////////////////////////////////////
        /// @brief 日志等级枚举，对应错误码中的 [b] 段（1 位 16 进制）
        /// @note  与 Log 的输出前缀一一对应：Info/Warning/Error/Fatal
        enum class LogLevel : uint32_t
        {
            Info    = 1, // 信息
            Warning = 2, // 警告
            Error   = 3, // 错误
            Fatal   = 4, // 严重错误（会终止程序）
        };

        /// @brief 模块号常量，对应错误码中的 [aa] 段（2 位 16 进制）
        namespace Module
        {
            constexpr uint32_t Unknown = 0x00; // 未知模块
            constexpr uint32_t Common  = 0x01; // 通用模块（测试用）
            constexpr uint32_t KFIO    = 0x02; // KFIO 文件读写模块
            constexpr uint32_t KSON    = 0x03; // KSON 解析模块
            constexpr uint32_t KTIMER  = 0x04; // KTIMER 计时模块
            constexpr uint32_t KCLI    = 0x05; // KCLI 命令行交互模块
            constexpr uint32_t KBIGNUM = 0x02; // 大数模块
        }

        /// @param module 模块号 [aa]，2 位 16 进制（bit24-31）
        /// @param level  等级   [b]， 1 位 16 进制（bit20-23）
        /// @param type   类型   [cc]，2 位 16 进制（bit12-19）
        /// @param id     序号   [ddd]，3 位 16 进制（bit0-11）
        /// @return 组合后的 32 位错误码，格式 0x[aa][b][cc][ddd]
        /// @attention constexpr 使 KLOGGER.cpp 中的错误码定义可在编译期求值
        constexpr Code MakeCode(uint32_t module, LogLevel level, uint32_t type, uint32_t id) noexcept
        {
            return ((module & 0xFF) << 24)
                 | ((static_cast<uint32_t>(level) & 0xF) << 20)
                 | ((type & 0xFF) << 12)
                 | (id & 0xFFF);
        }

        // 通用 / 测试模块 (01)
        extern const Code TEST_INFO;   // 测试-信息
        extern const Code TEST_WARN;   // 测试-警告
        extern const Code TEST_ERROR;  // 测试-错误
        extern const Code TEST_FATAL;  // 测试-严重错误
        extern const Code SYSTEM_OOM;  // 系统内存不足 FATAL
        // KFIO 模块 (02)
        extern const Code KFIO_FILE_OPEN_FAIL; // KFIO 文件打开失败 FATAL
        extern const Code KFIO_FILE_READ_FAIL; // KFIO 文件读取失败 FATAL

        // KCLI 模块 (05)
        extern const Code KCLI_INPUT_INVALID;   // KCLI 输入解析失败（类型不匹配）

        // KTIMER 模块 (04)
        extern const Code KTIMER_NOT_FOUND;       // KTIMER 计时器不存在 Warning
        extern const Code KTIMER_ALREADY_EXISTS;  // KTIMER 计时器已存在（将被覆盖） Warning
        extern const Code KTIMER_STATE_ERROR;     // KTIMER 计时器状态不允许此操作 Warning

        // KSON 模块 (03)
        extern const Code KSON_PARSE_STRE;             // 解析错误 不以双引号开头(多半是BUG)
        extern const Code KSON_PARSE_STR_NOEND;        // 解析错误 没有双引号匹配
        extern const Code KSON_PARSE_MULPOINT;         // 解析警告 多余的小数点.
        extern const Code KSON_PARSE_NUM_UE;           // 解析警告 数字中有不支持的非阿拉伯数字
        extern const Code KSON_PARSE_NUMOR;            // 解析错误 数字超出类型范围
        extern const Code KSON_PARSE_NUM_USTYPE;       // 解析错误 返回暂不支持的数字类型
        extern const Code KSON_PARSE_ESCAPE_SPECIAL;   // 解析警告 转义字符后接非特殊字符
        extern const Code KSON_PARSE_UNFINISHED_ESCAPE;// 解析错误 解析转义时越界了
        extern const Code KSON_PARSE_BIG_EXP;          // 解析错误 科学计数法指数过大
        extern const Code KSON_PARSE_VAL_END;          // 解析错误 读到文件末尾仍缺值
        extern const Code KSON_PARSE_VAL_ERROR;        // 解析错误 值类型无法识别
        extern const Code KSON_PARSE_ARR_BEGIN;        // 解析错误 期望数组起始 [
        extern const Code KSON_PARSE_ARRUE;            // 解析错误 数组中出现非预期字符
        extern const Code KSON_PARSE_OBJ_BEGIN;        // 解析错误 期望对象起始 {
        extern const Code KSON_PARSE_OBJ_KEY_QUOTE;    // 解析错误 对象键未用双引号包裹
        extern const Code KSON_PARSE_OBJ_SEPERATOR;    // 解析错误 键后缺少冒号分隔符 :
        extern const Code KSON_PARSE_OBJUE;            // 解析错误 对象中出现非预期字符
        extern const Code KSON_PARSE_TRAIL;            // 解析警告 结尾有多余字符
        extern const Code KSON_TYPE_MISMATCH;          // AsSth 类型不匹配 FATAL
        //KBIGNUM 模块 (06)
        extern const Code KBIGNUM_MULPOINT;            // 解析警告 多余的小数点.
        extern const Code KBIGNUM_INVALIDCHAR;        // 解析警告 数字中有不支持的非阿拉伯数字
        extern const Code KBIGNUM_DIVBYZERO;        // 除零错误 ERROR
        // 未知模块 (00)
        extern const Code UNKNOWN;

        /////////////////////////////////////////////////////////
        // VT100 颜色常量（KBegin / Log 自动启用 VT100 处理）
        /////////////////////////////////////////////////////////
        namespace Color
        {
            constexpr const char* Reset   = "\033[0m";
            constexpr const char* Red     = "\033[31m";
            constexpr const char* Green   = "\033[32m";
            constexpr const char* Yellow  = "\033[33m";
            constexpr const char* Blue    = "\033[34m";
            constexpr const char* Magenta = "\033[35m";
            constexpr const char* Cyan    = "\033[36m";
            constexpr const char* LightYellow = "\033[93m"; // 亮黄色
            constexpr const char* Orange  = "\033[38;5;208m"; // 256 色橙
            constexpr const char* SkyBlue = "\033[38;5;75m";  // 256 色天蓝
            constexpr const char* Gray    = "\033[90m"; // 深灰色
            constexpr const char* LightGray = "\033[38;5;250m"; // 浅灰色
            constexpr const char* Bold    = "\033[1m";
        }
    }
    namespace KBIGNUM
    {
        class BigNum; // 前向声明，供自由函数签名使用

        /// @brief 面向内部的运算（自由函数）
        slimb  AbsCmp(const BigNum& a, const BigNum& b); // 1 a>b ; 0 a=b; -1 a<b
        BigNum ScaleTo(const BigNum& x, size_t newScale); // 对齐小数位（数值不变，要求 newScale >= x.scale）
        BigNum AbsAdd(const BigNum& a, const BigNum& b);
        BigNum AbsSub(const BigNum& a, const BigNum& b);
        BigNum AbsMul(const BigNum& a, const BigNum& b);
        BigNum AbsDiv(const BigNum& a, const BigNum& b, size_t keep = 9); // 除法：a、b 均为整数且能整除 → 整数，否则保留 keep 位小数（默认 9）
        BigNum AbsMod(const BigNum& a, const BigNum& b); // 取模（除法取余）：abs(a) mod abs(b)

        BigNum AbsMulSchool(const BigNum& a, const BigNum& b); // 乘法（朴素算法）
        BigNum AbsMulKaratsuba(const BigNum& a, const BigNum& b); // 乘法（Karatsuba 算法）
        BigNum AbsMulToomCook3(const BigNum& a, const BigNum& b); // 乘法（Toom-Cook 3 算法）
        BigNum AbsMulNTT(const BigNum& a, const BigNum& b); // 乘法（NTT 算法）

        BigNum AbsDivSchool(const BigNum& a, const BigNum& b, size_t keep = 9); // 除法（朴素算法），保留 keep 位小数
        BigNum Pow(const BigNum& a, const BigNum& b); // 幂运算：a^b（快速幂，支持负指数/分数指数/inf-nan 规则）
        BigNum Root(const BigNum& a, const BigNum& n); // n 次方根：a^(1/n)（默认平方根），保留 9 位小数
        class BigNum
        {
            public:
                std::vector<limb> limbs = {0}; // 存储 (无小数点 无符号)
                /// @attention base = 10 ^ 9 ,之所以不用 2^32 是因为这 ToStr 太麻烦且太慢
                bool isneg = false; // 是否为负数
                size_t scale = 0; // 小数位数

                /// @brief 特殊状态
                enum class State { Normal, Inf, NegInf, Nan };
                State state = State::Normal; // Normal 普通 / Inf 正无穷 / NegInf 负无穷 / Nan 非数

                bool IsInf()    const { return state == State::Inf  || state == State::NegInf; } // ±无穷
                bool IsNan()    const { return state == State::Nan; }                             // 非数
                bool IsNormal() const { return state == State::Normal; }                          // 普通数值

                /// @brief 数值类型描述："nan" / "inf" / "-inf" / "int" / "dec"
                std::string type() const;

                static BigNum ToBig(const std::string& str); // 字符串转大数
                std::string   ToStr() const; // 大数转字符串

                /// @brief 构造 支持空 字符串 数字 特殊状态
                BigNum() = default;
                BigNum(const std::string& str);// 用字符串构造（inf/-inf/nan 大小写不敏感）
                /// @brief 用算术类型构造（int/long/double/float 等，自动正确处理负数/小数）
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum(const T& num) { *this = FromArith(num); }
                explicit BigNum(State s); // 特殊状态构造（Inf / NegInf / Nan）

                /// @brief 面向用户的运算
                BigNum operator+(const BigNum& b) const;
                BigNum operator-(const BigNum& b) const;
                BigNum operator*(const BigNum& b) const;
                BigNum operator/(const BigNum& b) const;
                BigNum operator%(const BigNum& b) const;

                /// @brief 与任意算术类型的运算（右值/变量均可，自动正确转换负数/小数）
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator+(const T& num) const { return *this + FromArith(num); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator-(const T& num) const { return *this - FromArith(num); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator*(const T& num) const { return *this * FromArith(num); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator/(const T& num) const { return *this / FromArith(num); }

                /// @brief 反向运算：算术类型 op BigNum（如 5 + bn、3.5 * bn）
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator+(const T& a, const BigNum& b) { return b + a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator-(const T& a, const BigNum& b) { return FromArith(a) - b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator*(const T& a, const BigNum& b) { return b * a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator/(const T& a, const BigNum& b) { return FromArith(a) / b; }
                /// @brief 比较 (IEEE-754：NaN 与任何比较均返回 false)
                bool operator==(const BigNum& b) const
                {
                    if(state == State::Nan || b.state == State::Nan) return false;
                    if(state != State::Normal || b.state != State::Normal)
                        return state == b.state; // inf == inf, -inf == -inf, 其余不等
                    return !(*this < b) && !(b < *this);
                }
                bool operator!=(const BigNum& b) const
                {
                    return !(*this == b); // NaN 时 == 为 false，故 != 为 true（IEEE-754）
                }
                bool operator<(const BigNum& b) const
                {
                    if(state == State::Nan || b.state == State::Nan) return false; // NaN 比较返回 false
                    // 特殊状态比较：-inf < normal < inf
                    if(state == State::NegInf) return b.state != State::NegInf; // -inf 小于所有非 -inf
                    if(b.state == State::Inf)  return state != State::Inf;      // 所有非 inf 小于 inf
                    if(state == State::Inf)    return false;                    // inf 不小于任何数
                    if(b.state == State::NegInf) return false;                  // 任何数不小于 -inf
                    // 以下均为 Normal
                    if(isneg && !b.isneg) return true;
                    if(!isneg && b.isneg) return false;
                    // 同号：对齐小数位后按绝对值比较
                    const size_t smax = (std::max)(scale, b.scale);
                    const slimb c = AbsCmp(ScaleTo(*this, smax), ScaleTo(b, smax));
                    return c == (isneg ? 1 : -1);
                }
                bool operator<=(const BigNum& b) const
                {
                    if(state == State::Nan || b.state == State::Nan) return false;
                    return (*this < b) || (*this == b);
                }
                bool operator>(const BigNum& b) const
                {
                    if(state == State::Nan || b.state == State::Nan) return false;
                    return !(*this <= b);
                }
                bool operator>=(const BigNum& b) const
                {
                    if(state == State::Nan || b.state == State::Nan) return false;
                    return !(*this < b);
                }

                /// @brief 输出
                friend std::ostream& operator<<(std::ostream& os, const BigNum& b)
                {
                    os << b.ToStr();
                    return os;
                }

                /// @brief 通过cin构造 BigNum（按空白分隔读取单个数，可用空格/换行隔开多个）
                friend std::istream& operator>>(std::istream& is, BigNum& b)
                {
                    std::string token;
                    if (!(is >> token)) return is;
                    b = BigNum(token);
                    return is;
                }

            private:
                /// @brief 算术类型 → BigNum（字符串转换，正确处理负数/小数）
                template<typename T>
                static BigNum FromArith(T v)
                {
                    if constexpr (std::is_integral_v<T>)
                        return BigNum(std::to_string(v));
                    else
                    {
                        std::ostringstream oss;
                        oss << std::setprecision(std::numeric_limits<T>::max_digits10) << v;
                        return BigNum(oss.str());
                    }
                }
        };
        std::string Normalize(const std::string& str); //合法化 包括但不限于去小数点 去前后导0
        BigNum RandBigNum(std::pair<size_t,size_t> IntRand={0,0}, std::pair<size_t,size_t> DecRand={0,0}, int sign=0); // 生成随机大数(整数位数范围 小数位数范围 符号:0随机/1全正/2全负)
    }
    namespace KSON
    {
        class NodePtr;
        enum class NodeType // 节点
        {
            kInt, // Integer 整数
            kDec, // Decimal 浮点数
            kBig, // BigNum 大数
            kStr, // String 字符串
            kBool, // Boolean 布尔值
            kArr, // Array 数组
            kObj, // Object 对象
            kNull, // Null 空值
        };
        class Node
        {
            public:
                // 构造
                Node() noexcept;
                explicit Node(bool val) noexcept;
                explicit Node(long long val) noexcept;
                explicit Node(double val) noexcept;
                explicit Node(KBIGNUM::BigNum val) noexcept;
                explicit Node(std::string val) noexcept;
                explicit Node(std::vector<Node> val);
                explicit Node(std::vector<std::pair<std::string,Node>> val);

                // 类型
                NodeType type()  const noexcept;
                bool IsNull()    const noexcept;
                bool IsBool()    const noexcept;
                bool IsInt()     const noexcept;
                bool IsDec()     const noexcept;
                bool IsBig()     const noexcept;
                bool IsNumber()  const noexcept;  // int 或 dec 或 big
                bool IsString()  const noexcept;
                bool IsArray()   const noexcept;
                bool IsObject()  const noexcept;

                // 取值
                bool             AsBool()   const;
                long long        AsInt()    const;
                double           AsDec() const;
                const KBIGNUM::BigNum& AsBig() const;
                std::string_view AsStr() const;
                const std::vector<Node>&     AsArr()  const;
                const std::vector<std::pair<std::string,Node>>&     AsObj() const;

                // 大小
                std::size_t size() const;
                
                // 查找（返回指针）
                const Node* find(std::string_view key) const; // 根据 键 查找对象中的键值对
                const Node* at(std::size_t index)      const; // 根据 下标 查找数组中的元素

                private:
                    // StorageType 节点存储类型 
                    /// @attention 与 NodeType 的区别是 : NodeType 是对外的

                    using arr_t = std::vector<Node>;
                    using obj_t = std::vector<std::pair<std::string,Node>>;
                    using storage_t = std::variant< 
                        std::monostate,
                        bool,std::string,double,long long,
                        KBIGNUM::BigNum,
                        arr_t,obj_t>;

                    storage_t Data;
        };
        struct PathSeg
        /// @attention 与 NodePtr 的区别是 : PathSeg 只记录一个位置片段，NodePtr 是一个完整的路径
        {
            std::string key;
            std::size_t index;
            PathSeg(std::string Key);
            PathSeg(std::size_t Index);
        };
        /// @brief KSON 树的指针，持有路径，提供访问方法
        class NodePtr
        {
            public:
                NodePtr() noexcept;
                explicit NodePtr(std::shared_ptr<Node> root) noexcept;
                NodePtr(std::shared_ptr<Node> root, std::vector<PathSeg> path) noexcept;
                
                static NodePtr Parse(std::string_view text);
                static NodePtr ParseFile(std::string_view filepath);
                
                NodePtr operator[](std::string_view key) const;
                NodePtr operator[](std::size_t index) const;
                NodePtr operator[](const char* key) const;
                
                const Node* TryResolve() const;
                const Node* Resolve() const;
                
                std::string Str() const;
                long long Int() const;
                double Dec() const;
                KBIGNUM::BigNum Big() const;
                bool Bool() const;
                std::size_t Size() const;
                std::size_t size() const;  // 小写别名，等价于 Size()，方便 arr.size() 风格
                bool Exists() const;

                /// @brief 自动分析值的类型，返回可打印的字符串表示
                /// @return 根据节点类型自动转换：
                ///         null→"null"  bool→"true"/"false"  int→数字串
                ///         dec→浮点串   str→字符串原文
                ///         arr→[e1, e2, ...]  obj→{"k": v, ...}
                /// @note  路径未找到时返回 "null"，不会 Fatal
                std::string Auto() const;
                
            private:
                std::shared_ptr<Node> root_;
                std::vector<PathSeg> path_;
                mutable const Node* cached_ = nullptr;
                
                const Node* ResolvePath(const std::vector<PathSeg>& path) const;
        };
        using kson = NodePtr;

        /////////////////////////////////////////////////////////

        ///@brief 读取文件并解析
        std::string Preprocess(std::string raw); // 预处理，将注释删除，将转义字符替换，去掉空格 换行等
        kson read(std::string_view processed);
        kson ReadKsonFile(std::string_view filename);
    }
    /////////////////////////////////////////////////////////
    // MazeCell 枚举 + 迷宫字符常量（共享于 KFIO / KCLI）
    /////////////////////////////////////////////////////////
    constexpr char MAZE_WALL  = 'W';   // 墙字符
    constexpr char MAZE_PATH  = 'P';   // 通路字符
    constexpr char MAZE_START = 'S';   // 起点字符
    constexpr char MAZE_END   = 'E';   // 终点字符
    enum MazeCell
    {
        WALL,      // 墙
        PASSABLE,  // 可通行
        VISITED,   // 已访问
        START,     // 起点
        END,       // 终点
        PATH       // 最终路径
    };
    namespace KFIO
    {
        std::string ReadFileRaw(std::string_view filepath);// 读取文件(粗文本 没有任何处理)

        /// @brief 从 KSON 文件读取迷宫
        /// @param filepath  KSON 文件路径
        /// @param maze_key  迷宫键名（对应 maze.kson 中的 "small" 等）
        /// @return 二维 MazeCell 网格
        std::vector<std::vector<MazeCell>> ReadMaze(
            std::string_view filepath,
            std::string_view maze_key = "small"
        );
    }
    namespace KCLI
    {
        // Color 常量统一定义在 KF::KLOGGER::Color，此处创建别名以便 KCLI 内直接使用 Color::xxx
        namespace Color = KF::KLOGGER::Color;

        /// @brief 链式输出流，自带默认颜色
        /// @details 每次 << 自动套用默认色；遇到 std::endl 等 manipulator 时
        ///          先输出 Color::Reset 再输出 manipulator，防止颜色泄漏。
        ///          支持在字符串中使用 {tag} 格式的颜色标签：
        ///          {red} {green} {blue} {yellow} {skyblue} {orange} {magenta} {cyan} {lightyellow}
        ///          {bold} {dim} {underline} {blink}
        ///          {bg_red} {bg_blue} ...
        ///          {/} 重置所有, {/bold} 重置单个
        /// @code
        /// kout  << "{red}红字{/} 普通信息" << std::endl;
        /// koutW << "{bold}{yellow}警告{/} 信息" << std::endl;
        /// @endcode
        class Kout
        {
            const char* color_;
        public:
            explicit constexpr Kout(const char* c) noexcept : color_(c) {}

            // 核心 tag 解析：扫描字符串，替换 {tag} 为 ANSI 码
            static std::string ParseTags(std::string_view str);

            template<typename T>
            Kout& operator<<(const T& val)
            {
                std::cout << color_ << val;
                return *this;
            }

            // 针对 std::string 特化，自动解析 {tag}
            Kout& operator<<(const std::string& val)
            {
                std::cout << color_ << ParseTags(val);
                return *this;
            }

            // 针对 const char* 特化
            Kout& operator<<(const char* val)
            {
                std::cout << color_ << ParseTags(std::string_view(val));
                return *this;
            }

            /// manipulator（std::endl / std::flush）：先 Reset 再输出
            Kout& operator<<(std::ostream& (*manip)(std::ostream&))
            {
                std::cout << Color::Reset << manip;
                return *this;
            }
        };

        /// @brief 链式输入会话：\c kin >> a >> b >> c 读入一组（空格/换行分隔）值，
        ///        自动按变量类型转换；任一非法则丢弃整组、给出醒目提示并整组重新输入，直到全部合法。
        /// @note  以分号结尾的每条 \c kin >> ... 语句为一组，值的个数须与变量个数一致。
        class KinSession
        {
            std::vector<std::function<bool(const std::string&)>> setters_;
        public:
            KinSession() = default;
            KinSession(const KinSession&) = delete;
            KinSession(KinSession&&) = default;
            KinSession& operator=(const KinSession&) = delete;
            KinSession& operator=(KinSession&&) = default;
            ~KinSession() { commit(); }

            /// 收集一个变量及其按类型生成的解析器
            template<typename T>
            KinSession&& operator>>(T& val)
            {
                setters_.push_back(makeSetter(val));
                return std::move(*this);
            }

        private:
            /// 为不同类型的变量生成「token -> bool(是否合法)」的解析器
            template<typename T>
            static std::function<bool(const std::string&)> makeSetter(T& val)
            {
                if constexpr (std::is_same_v<T, ::KF::KBIGNUM::BigNum>)
                {
                    return [&val](const std::string& t) -> bool {
                        try { val = ::KF::KBIGNUM::BigNum(t); return true; }
                        catch (...) { return false; }
                    };
                }
                else if constexpr (std::is_same_v<T, std::string>)// 如果是字符串
                {
                    return [&val](const std::string& t) -> bool { val = t; return true; };
                }
                else if constexpr (std::is_same_v<T, bool>) // 如果是布尔值
                {
                    // 大小写不敏感；支持常见真/假写法
                    return [&val](const std::string& t) -> bool
                    {
                        std::string low = t;
                        const std::vector<std::string> trueStrs = {"1", "true", "yes", "y", "on","ture","t"};
                        const std::vector<std::string> falseStrs = {"0", "false", "no", "n", "off","flase","f"};
                        for (auto& c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        if      (std::find(trueStrs.begin(), trueStrs.end(), low) != trueStrs.end()) { val = true;  return true; }
                        else if (std::find(falseStrs.begin(), falseStrs.end(), low) != falseStrs.end()) { val = false; return true; }
                        return false;
                    };
                }
                else if constexpr (std::is_integral_v<T>)
                {
                    // 严格整数：必须整个 token 都是整数，带小数点等视为非法
                    // 对于无符号类型 (size_t, uint, unsigned int 等)，只接受正整数，否则返回 false 走整组重试
                    return [&val](const std::string& t) -> bool {
                        try {
                            std::size_t idx = 0;
                            const long long v = std::stoll(t, &idx);
                            if (idx != t.size()) return false; // 如 "3.14" 解析到 '.' 即停，判非法
                            if constexpr (std::is_unsigned_v<T>) {
                                if (v < 0) return false; // 非正整数，走整组重试
                            }
                            val = static_cast<T>(v);
                            return true;
                        }
                        catch (...) { return false; }
                    };
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    return [&val](const std::string& t) -> bool {
                        try { val = static_cast<T>(std::stod(t)); return true; }
                        catch (...) { return false; }
                    };
                }
                else
                {
                    return [&val](const std::string&) -> bool { return false; };
                }
            }

            /// 整组读取：读够 N 个 token，全部合法才提交；否则醒目提示并整组重试
            void commit()
            {
                const std::size_t n = setters_.size();
                while (true)
                {
                    std::vector<std::string> tokens;
                    tokens.reserve(n);
                    while (tokens.size() < n)
                    {
                        std::string t;
                        if (!(std::cin >> t)) return; // EOF：放弃本次输入
                        tokens.push_back(t);
                    }
                    bool allok = true;
                    for (std::size_t i = 0; i < n; ++i)
                        if (!setters_[i](tokens[i])) { allok = false; break; }
                    if (allok) return;
                    // 醒目提示，整组重新输入
                    Kout(Color::Orange) << "\n输入不合法，请重新输入整组（以空格分隔，共 " << n << " 个值）：" << std::endl;
                }
            }
        };

        /// @brief 链式输入流，\c kin >> a >> b >> c 自动按变量类型读取并整组校验
        /// @code int x; bool b; kin >> x >> b; @endcode
        class Kin
        {
        public:
            template<typename T>
            KinSession operator>>(T& val)
            {
                KinSession s;
                s >> val;
                return std::move(s);
            }
        };

        inline Kout kout(Color::Bold);      ///< 默认天蓝色（普通输出）
        inline Kout koutW(Color::LightYellow); ///< 默认淡黄色（Warning）
        inline Kout koutE(Color::Orange);  ///< 默认橙色（Error）
        inline Kout koutF(Color::Red);     ///< 默认红色（Fatal）
        inline Kin kin;                    ///< 全局输入流对象（链式 >> ，自动推导类型）

        /////////////////////////////////////////////////////////
        // CLI 功能函数
        /////////////////////////////////////////////////////////

        /// @brief 初始化 CLI：启用 VT100 颜色，设置控制台标题（支持中文），
        ///        打印标题框和描述
        /// 字符串版本（可变参数，定义见下方模板）：
        ///   KBegin(title, desc)                  // 2 参
        ///   KBegin(title, desc, author)          // 3 参
        ///   KBegin(title, desc, author, date)    // 4 参
        /// @param title 标题（留空则不打印标题框）
        /// @param description 描述（留空则不打印描述）
        /// @param author 作者（亮黄显示）
        /// @param date 日期（亮黄显示）
        void KBeginImpl(const std::string& cmdtitle, const std::string& title,
                        const std::string& description, const std::string& author,
                        const std::string& date);

        template<typename... Args>
        void KBegin(const std::string& a, const std::string& b, Args... rest)
        {
            std::vector<std::string> args{a, b, rest...};
            // 语义: [title, desc] / [title, desc, author] / [title, desc, author, date]
            std::string title = a;
            std::string description = b;
            std::string author = args.size() >= 3 ? args[2] : "";
            std::string date   = args.size() >= 4 ? args[3] : "";
            KBeginImpl(title, title, description, author, date);
        }

        void KBegin(const KSON::kson file);//从文件中读取

        /// @brief 全局配置：KBegin 时自动读取 config/global.kson 的 data 节点
        /// @details 程序内任意位置可直接使用 GLOBAL["键"] 读取全局配置项
        extern KSON::kson GLOBAL;
        /// @brief 显示选项菜单，循环等待用户输入合法选项
        /// @param menu KSON 节点，需含 "title" 和 "options"（字符串数组）
        /// @return 选中项索引（0-based），输入非法时循环提示
        std::size_t KOptions(const KSON::kson& menu);

        /// @brief 暂停：显示"按任意键继续..."并等待按键
        void KPause();

        /// @brief 结束：暂停后退出程序（exit(0)）
        void KEnd();
        ////////////////////////////////////////杂函数/////////////////////////

        /// @brief 迷宫可视化打印（全量刷新，内部含 \033[H/\033[K/\033[J）
        class Maze
        {
            public:
                static void Print(const std::vector<std::vector<MazeCell>>& maze);
        };

        /// @brief 数组柱状图打印（基于序号，不显示数字，柱状条用 #）
        class Arr
        {
            public:
                static void Print(const std::vector<size_t>& ranks, size_t n,
                                int barMax = 50, int highlight1 = -1,
                                int highlight2 = -1, int sortedUntil = -1);
        };
        /// @brief 经典 cmd 下自动分级适配（先调窗口→缩字号到 6→全屏）；keepIfFits=true 时内容过小则保持原窗口
        /// @param rows 需要显示的行数
        /// @param cols 需要显示的列数
        /// @param keepIfFits 内容过小时保持原窗口（迷宫场景传 true）
        /// @return 恒为 true（cmd 下总是尽力适配）
        bool CheckConsoleFit(int rows, int cols, bool keepIfFits = false);

        /// @brief ANSI 清屏（光标归位+清除屏幕），替代 system("cls") 避免闪屏
        void ClearScreen();
    }
    namespace KTIMER
    {
        // Color 常量统一定义在 KF::KLOGGER::Color，此处创建别名以便 KTIMER 内直接使用 Color::xxx
        namespace Color = KF::KLOGGER::Color;

        /// @brief 时间单位枚举
        enum class TimeUnit
        {
            ns,  // 纳秒
            us,  // 微秒
            ms,  // 毫秒
            s,   // 秒
        };

        /// @brief 计时器状态枚举
        enum class TimerState
        {
            Running,  // 运行中
            Paused,   // 已暂停
        };

        /// @brief 新建计时器（指定名字和单位），创建后立即开始计时
        /// @param name  计时器名称（唯一标识）
        /// @param unit  时间单位（ns/us/ms/s）
        /// @return true=新建成功，false=同名已存在（已覆盖，发出警告）
        bool AddTimer(const std::string& name, TimeUnit unit);

        /// @brief 暂停计时器（累计已运行时间）
        /// @return true=暂停成功，false=不存在或未在运行
        bool PauseTimer(const std::string& name);

        /// @brief 恢复已暂停的计时器
        /// @return true=恢复成功，false=不存在或未暂停
        bool StartTimer(const std::string& name);

        /// @brief 删除计时器
        /// @return true=删除成功，false=不存在
        bool DeleteTimer(const std::string& name);

        /// @brief 获取计时器当前累计时间（按计时器单位）
        /// @return >=0 累计时间，-1.0 表示不存在
        double GetTimer(const std::string& name);

        /// @brief 打印单个计时器信息（格式化框）
        void PrintTimer(const std::string& name);

        /// @brief 打印所有计时器信息（格式化表格，按名称排序）
        void PrintAllTimers();
    }
    /// @brief 实用库
    namespace KUTIL
    {
        sdlimb RandInt(sdlimb min, sdlimb max); ///< 生成 [min, max] 范围内的随机整数
        sdlimb Pow10(sdlimb n); // 10的n次方
        std::string MaxLenStr3(std::string a, std::string b, std::string c);
        std::string MaxLenStr4(std::string a, std::string b, std::string c, std::string d);
    }
}
namespace KSON = KF::KSON;
namespace KLOG = KF::KLOGGER;
namespace KFIO = KF::KFIO;
namespace KUTIL = KF::KUTIL;
namespace KCLI = KF::KCLI;
namespace KTIMER = KF::KTIMER;
namespace KBIGNUM = KF::KBIGNUM;
constexpr size_t DEFAULT_RESIZE_STR_LEN = 64; // 默认KSON中字符串的分配长度 (超过这个长度会再次扩容)
using namespace KF::KLOGGER;