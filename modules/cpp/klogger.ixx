module;
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstdlib>

export module klogger;

namespace  // 模块号常量（错误码 [aa] 段），私有，不导出
{
    constexpr uint32_t Unknown = 0x00;
    constexpr uint32_t Common  = 0x01;
    constexpr uint32_t KSON    = 0x03; // KSON 解析 + 文件读写
    constexpr uint32_t KTIMER  = 0x04; // KTIMER 计时模块
    constexpr uint32_t KCLI    = 0x05; // KCLI 命令行交互模块
    constexpr uint32_t KBIGNUM = 0x06; // KBIGNUM 大数运算模块
}

export
{
    using Code = uint32_t;
    using limb = uint32_t;   // 基础分块
    using dlimb = uint64_t;  // double limb
    using slimb = int32_t;   // signed limb
    using sdlimb = int64_t;  // signed double limb

    /// @brief 日志等级枚举
    enum class LogLevel : uint32_t
    {
        Info    = 1,
        Warning = 2,
        Error   = 3,
        Fatal   = 4,
    };

    /// @brief 组合错误码：0x[aa][b][cc][ddd]
    constexpr Code MakeCode(uint32_t module, LogLevel level, uint32_t type, uint32_t id) noexcept
    {
        return ((module & 0xFF) << 24)
             | ((static_cast<uint32_t>(level) & 0xF) << 20)
             | ((type & 0xFF) << 12)
             | (id & 0xFFF);
    }

    /////////////////////////////////////////////////////////
    // VT100 颜色常量
    /////////////////////////////////////////////////////////
    inline constexpr const char* Reset   = "\033[0m";
    inline constexpr const char* Red     = "\033[31m";
    inline constexpr const char* Green   = "\033[32m";
    inline constexpr const char* Yellow  = "\033[33m";
    inline constexpr const char* Blue    = "\033[34m";
    inline constexpr const char* Magenta = "\033[35m";
    inline constexpr const char* Cyan    = "\033[36m";
    inline constexpr const char* LightYellow = "\033[93m";
    inline constexpr const char* Orange  = "\033[38;5;208m";
    inline constexpr const char* SkyBlue = "\033[38;5;75m";
    inline constexpr const char* Gray    = "\033[90m";
    inline constexpr const char* LightGray = "\033[38;5;250m";
    inline constexpr const char* Bold    = "\033[1m";

    void Error(Code code, const std::string& extra, const char* file, int line, const char* func);
    void Warning(Code code, const std::string& extra, const char* file, int line, const char* func);
    void Info(Code code, const std::string& extra, const char* file, int line, const char* func);
    void Fatal(Code code, const std::string& extra, const char* file, int line, const char* func);

    extern std::unordered_map<Code, std::string_view> Table;

    /////////////////////////////////////////////////////////
    // 错误码：全部集中在 klogger，无 extern、无分散定义
    /////////////////////////////////////////////////////////
    constexpr Code TEST_INFO   = MakeCode(Common, LogLevel::Info,    0x01, 0x001);
    constexpr Code TEST_WARN   = MakeCode(Common, LogLevel::Warning, 0x01, 0x002);
    constexpr Code TEST_ERROR  = MakeCode(Common, LogLevel::Error,   0x01, 0x003);
    constexpr Code TEST_FATAL  = MakeCode(Common, LogLevel::Fatal,   0x01, 0x004);
    constexpr Code SYSTEM_OOM  = MakeCode(Common, LogLevel::Fatal,   0x01, 0x005);

    constexpr Code KSON_FILE_OPEN_FAIL = MakeCode(KSON, LogLevel::Fatal, 0x02, 0x001);
    constexpr Code KSON_FILE_READ_FAIL = MakeCode(KSON, LogLevel::Fatal, 0x02, 0x002);

    constexpr Code KCLI_INPUT_INVALID = MakeCode(KCLI, LogLevel::Warning, 0x01, 0x001);

    constexpr Code KTIMER_NOT_FOUND      = MakeCode(KTIMER, LogLevel::Warning, 0x01, 0x001);
    constexpr Code KTIMER_ALREADY_EXISTS = MakeCode(KTIMER, LogLevel::Warning, 0x01, 0x002);
    constexpr Code KTIMER_STATE_ERROR    = MakeCode(KTIMER, LogLevel::Warning, 0x01, 0x003);

    constexpr Code KSON_PARSE_STRE              = MakeCode(KSON, LogLevel::Error,   0x01, 0x001);
    constexpr Code KSON_PARSE_STR_NOEND         = MakeCode(KSON, LogLevel::Error,   0x01, 0x002);
    constexpr Code KSON_PARSE_MULPOINT          = MakeCode(KSON, LogLevel::Warning, 0x01, 0x003);
    constexpr Code KSON_PARSE_NUM_UE            = MakeCode(KSON, LogLevel::Warning, 0x01, 0x004);
    constexpr Code KSON_PARSE_NUMOR             = MakeCode(KSON, LogLevel::Error,   0x01, 0x005);
    constexpr Code KSON_PARSE_NUM_USTYPE        = MakeCode(KSON, LogLevel::Error,   0x01, 0x006);
    constexpr Code KSON_PARSE_ESCAPE_SPECIAL    = MakeCode(KSON, LogLevel::Warning, 0x01, 0x007);
    constexpr Code KSON_PARSE_UNFINISHED_ESCAPE = MakeCode(KSON, LogLevel::Fatal,   0x01, 0x00A);
    constexpr Code KSON_PARSE_BIG_EXP           = MakeCode(KSON, LogLevel::Error,   0x01, 0x00C);
    constexpr Code KSON_PARSE_VAL_END           = MakeCode(KSON, LogLevel::Error,   0x01, 0x0AA);
    constexpr Code KSON_PARSE_VAL_ERROR         = MakeCode(KSON, LogLevel::Error,   0x01, 0x0AC);
    constexpr Code KSON_PARSE_ARR_BEGIN         = MakeCode(KSON, LogLevel::Error,   0x01, 0x0AE);
    constexpr Code KSON_PARSE_ARRUE             = MakeCode(KSON, LogLevel::Error,   0x01, 0x0B0);
    constexpr Code KSON_PARSE_OBJ_BEGIN         = MakeCode(KSON, LogLevel::Error,   0x01, 0x0B2);
    constexpr Code KSON_PARSE_OBJ_KEY_QUOTE     = MakeCode(KSON, LogLevel::Error,   0x01, 0x0B4);
    constexpr Code KSON_PARSE_OBJ_SEPERATOR     = MakeCode(KSON, LogLevel::Error,   0x01, 0x0B6);
    constexpr Code KSON_PARSE_OBJUE             = MakeCode(KSON, LogLevel::Error,   0x01, 0x0B8);
    constexpr Code KSON_PARSE_TRAIL             = MakeCode(KSON, LogLevel::Warning, 0x02, 0x011);
    constexpr Code KSON_TYPE_MISMATCH           = MakeCode(KSON, LogLevel::Fatal,   0x01, 0x001);

    constexpr Code KBIGNUM_MULPOINT   = MakeCode(KBIGNUM, LogLevel::Warning, 0x01, 0x002);
    constexpr Code KBIGNUM_INVALIDCHAR = MakeCode(KBIGNUM, LogLevel::Warning, 0x01, 0x004);
    constexpr Code KBIGNUM_DIVBYZERO  = MakeCode(KBIGNUM, LogLevel::Error, 0x02, 0x003);

    constexpr Code UNKNOWN = MakeCode(Unknown, LogLevel::Fatal, 0x00, 0x000);
}

    static const char* Basename(const char* path)
    {
        if (!path) return "?";
        const char* base = path;
        for (const char* p = path; *p; ++p)
            if (*p == '\\' || *p == '/') base = p + 1;
        return base;
    }

    void Log(Code code, const std::string& extra, LogLevel level,
             const char* file, int line, const char* func)
    {
        std::string desc = "未知错误码";
        auto iter = Table.find(code);
        if (iter != Table.end())
            desc = std::string(iter->second);
        auto flag = std::cerr.flags();
        char oldfill = std::cerr.fill();

        switch (level)
        {
            case LogLevel::Info:
                std::cerr << "\n" << Green << "[INFO]" << Reset << " ";
                break;
            case LogLevel::Warning:
                std::cerr << "\n" << LightYellow << "[WARNING]" << Reset << " ";
                break;
            case LogLevel::Error:
                std::cerr << "\n" << Orange << "[ERROR]" << Reset << " ";
                break;
            case LogLevel::Fatal:
                std::cerr << "\n" << Bold << Red << "[FATAL]" << Reset << " ";
                break;
            default:
                std::cerr << "\n[UNKNOWN] ";
        }

        std::cerr << " Code: 0x"
                  << std::setw(8) << std::setfill('0') << std::hex << code
                  << " Msg: " << desc;
        if (!extra.empty())
            std::cerr << " | " << extra;
        std::cerr << " | at " << Basename(file) << ":" << std::dec << line;
        if (func)
            std::cerr << " (" << func << ")";
        std::cerr << "\n";

        std::cerr.flags(flag);
        std::cerr.fill(oldfill);
    }

    void Error(Code code, const std::string& extra, const char* file, int line, const char* func)
    { Log(code, extra, LogLevel::Error, file, line, func); }

    void Warning(Code code, const std::string& extra, const char* file, int line, const char* func)
    { Log(code, extra, LogLevel::Warning, file, line, func); }

    void Info(Code code, const std::string& extra, const char* file, int line, const char* func)
    { Log(code, extra, LogLevel::Info, file, line, func); }

    void Fatal(Code code, const std::string& extra, const char* file, int line, const char* func)
    {
        Log(code, extra, LogLevel::Fatal, file, line, func);
        std::cerr << "\n程序将终止运行...\n";
        system("pause");
        exit(EXIT_FAILURE);
    }

    std::unordered_map<Code, std::string_view> Table =
    {
        {UNKNOWN,                          "Unknown,try to look the msg"},
        {TEST_INFO,                        "测试-信息"},
        {TEST_WARN,                        "测试-警告"},
        {TEST_ERROR,                       "测试-错误"},
        {TEST_FATAL,                       "测试-严重错误"},
        {SYSTEM_OOM,                       "out of memory,can not allocate memory"},
        {KSON_FILE_OPEN_FAIL,              "KSON open file failed"},
        {KSON_FILE_READ_FAIL,              "KSON read file failed"},
        {KCLI_INPUT_INVALID,               "KCLI input invalid, failed to parse"},
        {KTIMER_NOT_FOUND,                 "KTIMER timer not found"},
        {KTIMER_ALREADY_EXISTS,            "KTIMER timer already exists, will be overwritten"},
        {KTIMER_STATE_ERROR,               "KTIMER timer state does not allow this operation"},
        {KSON_PARSE_STRE,                  "KSON Parse string, expecting a quote"},
        {KSON_PARSE_STR_NOEND,             "KSON Parse string, expecting an ending quote"},
        {KSON_PARSE_MULPOINT,              "KSON Parse number, multiple decimal points"},
        {KSON_PARSE_NUM_UE,                "KSON Parse number, The number contains unsupported non-Arabic digits"},
        {KSON_PARSE_NUMOR,                 "KSON Parse number, Can't store this number, maybe it's out of range"},
        {KSON_PARSE_NUM_USTYPE,            "KSON Parse number, return an unsupported number type"},
        {KSON_PARSE_ESCAPE_SPECIAL,        "KSON Parse string, There's an invalid char following the escape char"},
        {KSON_PARSE_UNFINISHED_ESCAPE,     "KSON Parse string, out of range when reading an escape"},
        {KSON_PARSE_BIG_EXP,               "KSON Parse number, exponent too large for scientific notation"},
        {KSON_PARSE_VAL_END,               "KSON Parse value, the fileRead is ended"},
        {KSON_PARSE_VAL_ERROR,             "KSON Parse value, error occured"},
        {KSON_PARSE_ARR_BEGIN,             "KSON Parse array, expecting a opening bracket"},
        {KSON_PARSE_ARRUE,                 "KSON Parse array, unexpected char"},
        {KSON_PARSE_OBJ_BEGIN,             "KSON Parse object, expecting a opening brace"},
        {KSON_PARSE_OBJ_KEY_QUOTE,         "KSON Parse object, expected a quote"},
        {KSON_PARSE_OBJ_SEPERATOR,         "KSON Parse object, expected a colon separator"},
        {KSON_PARSE_OBJUE,                 "KSON Parse object, unexpected char"},
        {KSON_PARSE_TRAIL,                 "KSON Parse string, unexpected following char"},
        {KSON_TYPE_MISMATCH,               "KSON AsSth type mismatch"},
        {KBIGNUM_MULPOINT,                 "KBIGNUM Parse number, multiple decimal points"},
        {KBIGNUM_INVALIDCHAR,              "KBIGNUM Parse number, The number contains unsupported non-Arabic digits"},
        {KBIGNUM_DIVBYZERO,                "KBIGNUM divide by zero, result is +/-inf (0/0 is nan)"},
    };