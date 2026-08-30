/**
 * @file    dbgKLOGGER.cpp
 * @brief   KLOGGER 日志模块配置驱动测试
 *
 * 检测单元从 config/test/cfg.kson 的 dbgKLOGGER 读取：
 *   - codes:    错误码名列表，逐一展示 HexCode 与 Table 码表查询
 *   - triggers: 日志触发配置（code + msg），按码的等级动态触发
 *
 * 若配置键缺失则输出 {red}[FAIL]{/} 并继续，不会退出程序。
 */

#include "base/KF.hpp"
#include <unordered_map>
using namespace KFIO;
using namespace KSON;
using namespace KLOG;
using namespace KCLI;

// ==================== 测试辅助 ====================
#define SECTION(name) kout << Color::Bold << "\n--- " << name << " ---" << Color::Reset << std::endl

static int g_ok = 0, g_fail = 0;

// 辅助函数: 将 Code 转为 hex 字符串
static std::string HexCode(Code c)
{
    std::ostringstream oss;
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << c;
    return oss.str();
}

/// 错误码名 -> Code 常量 查找表（测试样本的码名统一在 cfg.kson 中定义）
static Code LookupCode(const std::string& name)
{
    static const std::unordered_map<std::string, Code> m =
    {
        { "TEST_INFO",              TEST_INFO },
        { "TEST_WARN",              TEST_WARN },
        { "TEST_ERROR",             TEST_ERROR },
        { "TEST_FATAL",             TEST_FATAL },
        { "KFIO_FILE_OPEN_FAIL",    KFIO_FILE_OPEN_FAIL },
        { "KFIO_FILE_READ_FAIL",    KFIO_FILE_READ_FAIL },
        { "KCLI_INPUT_INVALID",     KCLI_INPUT_INVALID },
        { "KTIMER_NOT_FOUND",       KTIMER_NOT_FOUND },
        { "KTIMER_ALREADY_EXISTS",  KTIMER_ALREADY_EXISTS },
        { "KTIMER_STATE_ERROR",     KTIMER_STATE_ERROR },
        { "KSON_PARSE_STRE",        KSON_PARSE_STRE },
        { "KSON_PARSE_STR_NOEND",   KSON_PARSE_STR_NOEND },
        { "KSON_PARSE_MULPOINT",    KSON_PARSE_MULPOINT },
        { "KSON_PARSE_NUM_UE",      KSON_PARSE_NUM_UE },
        { "KSON_PARSE_NUMOR",       KSON_PARSE_NUMOR },
        { "KSON_PARSE_NUM_USTYPE",  KSON_PARSE_NUM_USTYPE },
        { "KSON_PARSE_ESCAPE_SPECIAL", KSON_PARSE_ESCAPE_SPECIAL },
        { "KSON_PARSE_UNFINISHED_ESCAPE", KSON_PARSE_UNFINISHED_ESCAPE },
        { "KSON_PARSE_BIG_EXP",     KSON_PARSE_BIG_EXP },
        { "KSON_PARSE_VAL_END",     KSON_PARSE_VAL_END },
        { "KSON_PARSE_VAL_ERROR",   KSON_PARSE_VAL_ERROR },
        { "KSON_PARSE_ARR_BEGIN",   KSON_PARSE_ARR_BEGIN },
        { "KSON_PARSE_ARRUE",       KSON_PARSE_ARRUE },
        { "KSON_PARSE_OBJ_BEGIN",   KSON_PARSE_OBJ_BEGIN },
        { "KSON_PARSE_OBJ_KEY_QUOTE", KSON_PARSE_OBJ_KEY_QUOTE },
        { "KSON_PARSE_OBJ_SEPERATOR", KSON_PARSE_OBJ_SEPERATOR },
        { "KSON_PARSE_OBJUE",       KSON_PARSE_OBJUE },
        { "KSON_PARSE_TRAIL",       KSON_PARSE_TRAIL },
        { "KSON_TYPE_MISMATCH",     KSON_TYPE_MISMATCH },
        { "KMATH_MULPOINT",       KMATH_MULPOINT },
        { "KMATH_INVALIDCHAR",    KMATH_INVALIDCHAR },
        { "UNKNOWN",                UNKNOWN },
    };
    auto it = m.find(name);
    return it != m.end() ? it->second : static_cast<Code>(-1);
}

/// 从错误码中提取日志等级（bit20-23）
static LogLevel LevelOf(Code c)
{
    return static_cast<LogLevel>((c >> 20) & 0xF);
}

/// 按码的等级动态触发日志
static void Trigger(Code c, const std::string& msg)
{
    switch (LevelOf(c))
    {
        case LogLevel::Info:    KLOG_INFO(c, msg);    break;
        case LogLevel::Warning: KLOG_WARNING(c, msg); break;
        case LogLevel::Error:   KLOG_ERROR(c, msg);   break;
        case LogLevel::Fatal:   KLOG_WARNING(c, msg); break; // 配置中不触发 Fatal（会终止程序），降级为 Warning
        default:                KLOG_ERROR(c, msg);   break;
    }
}

int main()
{
    auto doc = ReadKsonFile("config/test/cfg.kson");
    auto logger = doc["dbgKLOGGER"];
    KBegin(logger["meta"].Exists() ? logger["meta"] : doc["dbgKSON"]["meta"]);

    // ==================== 1. codes 错误码展示 + Table 查询 ====================
    SECTION("1. codes 错误码 + Table 码表查询");
    if (!logger["codes"].Exists())
    {
        koutE << "{red}[FAIL]{/}  dbgKLOGGER 缺少 codes 键，跳过该节" << std::endl;
        ++g_fail;
    }
    else
    {
        auto codes = logger["codes"];
        for (size_t i = 0; i < codes.size(); i++)
        {
            std::string name = codes[i].Str();
            Code c = LookupCode(name);
            if (c == static_cast<Code>(-1))
            {
                kout << "  " << name << "  {red}[FAIL]{/}  无法解析为 Code" << std::endl;
                ++g_fail;
                continue;
            }
            auto it = Table.find(c);
            std::string desc = (it != Table.end()) ? std::string(it->second) : "(not in table)";
            kout << "  " << name << " = " << HexCode(c);
            if (it != Table.end())
                kout << " {green}[OK]{/}  -> " << desc;
            else
                kout << " {red}[FAIL]{/}  不在码表中，desc=" << desc;
            kout << std::endl;
            if (it != Table.end()) ++g_ok; else ++g_fail;
        }
    }

    // ==================== 2. triggers 日志触发 ====================
    SECTION("2. triggers 日志触发");
    if (!logger["triggers"].Exists())
    {
        koutE << "{red}[FAIL]{/}  dbgKLOGGER 缺少 triggers 键，跳过该节" << std::endl;
        ++g_fail;
    }
    else
    {
        auto triggers = logger["triggers"];
        for (size_t i = 0; i < triggers.size(); i++)
        {
            std::string name = triggers[i]["code"].Exists() ? triggers[i]["code"].Str() : "";
            std::string msg  = triggers[i]["msg"].Exists()  ? triggers[i]["msg"].Str()  : "";
            if (!triggers[i]["code"].Exists())
            {
                kout << "  trigger[" << i << "] {red}[FAIL]{/}  缺少 code 字段" << std::endl;
                ++g_fail;
                continue;
            }
            Code c = LookupCode(name);
            if (c == static_cast<Code>(-1))
            {
                kout << "  trigger[" << i << "] {red}[FAIL]{/}  未知码名 \"" << name << "\"" << std::endl;
                ++g_fail;
                continue;
            }
            kout << "  >> " << name << "(" << HexCode(c) << ") \"" << msg << "\"" << std::endl;
            Trigger(c, msg);
            ++g_ok;
        }
    }

    // ==================== 3. MakeCode 错误码组装 ====================
    SECTION("3. MakeCode 错误码组装");
    {
        Code manual = MakeCode(Module::KSON, LogLevel::Error, 0x01, 0x001);
        kout << "  MakeCode(KSON, Error, 0x01, 0x001) = " << HexCode(manual) << std::endl;
        if (manual == KSON_PARSE_STRE)
            kout << "  == KSON_PARSE_STRE {green}[OK]{/}" << std::endl, ++g_ok;
        else
            kout << "  == KSON_PARSE_STRE {red}[FAIL]{/}" << std::endl, ++g_fail;
    }

    // ==================== 4. LogLevel / Module 枚举 ====================
    SECTION("4. LogLevel / Module 枚举");
    {
        kout << "  LogLevel::Info    = " << static_cast<uint32_t>(LogLevel::Info)    << std::endl;
        kout << "  LogLevel::Warning = " << static_cast<uint32_t>(LogLevel::Warning) << std::endl;
        kout << "  LogLevel::Error   = " << static_cast<uint32_t>(LogLevel::Error)   << std::endl;
        kout << "  LogLevel::Fatal   = " << static_cast<uint32_t>(LogLevel::Fatal)   << std::endl;
        kout << "  Module::Unknown   = " << HexCode(Module::Unknown) << std::endl;
        kout << "  Module::Common    = " << HexCode(Module::Common)  << std::endl;
        kout << "  Module::KFIO      = " << HexCode(Module::KFIO)    << std::endl;
        kout << "  Module::KSON      = " << HexCode(Module::KSON)    << std::endl;
        kout << "  Module::KTIMER    = " << HexCode(Module::KTIMER)  << std::endl;
        kout << "  Module::KCLI      = " << HexCode(Module::KCLI)    << std::endl;
        kout << "  Module::KMATH   = " << HexCode(Module::KMATH) << std::endl;
    }

    // ==================== 5. Color 颜色常量展示 ====================
    SECTION("5. Color 颜色常量展示");
    {
        kout << Color::Red         << "  Red"          << Color::Reset << std::endl;
        kout << Color::Green       << "  Green"        << Color::Reset << std::endl;
        kout << Color::Yellow      << "  Yellow"       << Color::Reset << std::endl;
        kout << Color::Blue        << "  Blue"         << Color::Reset << std::endl;
        kout << Color::Magenta     << "  Magenta"      << Color::Reset << std::endl;
        kout << Color::Cyan        << "  Cyan"         << Color::Reset << std::endl;
        kout << Color::LightYellow << "  LightYellow"  << Color::Reset << std::endl;
        kout << Color::Orange      << "  Orange"       << Color::Reset << std::endl;
        kout << Color::SkyBlue     << "  SkyBlue"      << Color::Reset << std::endl;
        kout << Color::Bold        << "  Bold"         << Color::Reset << std::endl;
    }

    // ==================== 结论 ====================
    kout << "\n----------------------------------------\n";
    kout << "  {green}[OK]{/} " << g_ok << " 项通过\n";
    if (g_fail == 0)
        kout << "\n{green}[ALL PASS] KLOGGER 配置驱动测试通过{/}\n";
    else
        kout << "\n{red}[" << g_fail << " FAIL] KLOGGER 配置驱动测试有失败项{/}\n";

    KEnd();
    return g_fail > 0 ? 1 : 0;
}