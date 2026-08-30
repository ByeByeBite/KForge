#include "base/KF.hpp"

/**
 * @file KLOGGER.cpp
 * @brief 日志模块
 * @version 1.0.0
 * @date 2026-08-13
 * @author Git-1145
 */

namespace KF
{
    namespace KLOGGER
    {
        /// @brief 从完整路径中提取文件名（去掉目录部分）
        static const char* Basename(const char* path)
        {
            if (!path) return "?";
            const char* base = path;
            for (const char* p = path; *p; ++p)
                if (*p == '\\' || *p == '/') base = p + 1;
            return base;
        }

        /**
         * @brief 输出日志的核心函数，根据错误码匹配说明文本并按等级打印前缀
         * @param code  错误码（由 MakeCode 组装）
         * @param extra 附加自定义信息（可选）
         * @param level 日志等级，决定输出前缀 [INFO]/[WARNING]/[ERROR]/[FATAL]
         * @param file  源文件名（由宏 __FILE__ 捕获）
         * @param line  源行号（由宏 __LINE__ 捕获）
         * @param func  函数名（由宏 __FUNCTION__ 捕获）
         * @note  内部函数，头文件未声明，仅供 Error/Warning/Info/Fatal 包装调用
         */
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
                    std::cerr << "\n" << Color::Green << "[INFO]" << Color::Reset << " ";
                    break;
                case LogLevel::Warning:
                    std::cerr << "\n" << Color::LightYellow << "[WARNING]" << Color::Reset << " ";
                    break;
                case LogLevel::Error:
                    std::cerr << "\n" << Color::Orange << "[ERROR]" << Color::Reset << " ";
                    break;
                case LogLevel::Fatal:
                    std::cerr << "\n" << Color::Bold << Color::Red << "[FATAL]" << Color::Reset << " ";
                    break;
                default:
                    std::cerr << "\n[UNKNOWN] ";
            }

            std::cerr << " Code: 0x"
                    << std::setw(8) << std::setfill('0') << std::hex << code
                    << " Msg: " << desc;
            if (!extra.empty())
                std::cerr << " | " << extra;
            // 附带出错位置：文件名:行号 (函数名)
            std::cerr << " | at " << Basename(file) << ":" << std::dec << line;
            if (func)
                std::cerr << " (" << func << ")";
            std::cerr << "\n";

            // 恢复原始输出格式
            std::cerr.flags(flag);
            std::cerr.fill(oldfill);
        }

        /// @brief 输出 [ERROR] 级别日志（程序继续运行）
        void Error(Code code, const std::string& extra, const char* file, int line, const char* func)
        {
            Log(code, extra, LogLevel::Error, file, line, func);
        }
        /// @brief 输出 [WARNING] 级别日志
        void Warning(Code code, const std::string& extra, const char* file, int line, const char* func)
        {
            Log(code, extra, LogLevel::Warning, file, line, func);
        }
        /// @brief 输出 [INFO] 级别日志
        void Info(Code code, const std::string& extra, const char* file, int line, const char* func)
        {
            Log(code, extra, LogLevel::Info, file, line, func);
        }
        /// @brief 输出 [FATAL] 级别日志并终止程序
        /// @attention 调用后程序会 system("pause") 再 exit，不可恢复
        void Fatal(Code code, const std::string& extra, const char* file, int line, const char* func)
        {
            Log(code, extra, LogLevel::Fatal, file, line, func);
            std::cerr << "\n程序将终止运行...\n";
            system("pause");
            exit(EXIT_FAILURE);
        }

        /////////////////////////////////////////////////////////
        // 错误码定义（统一通过 MakeCode 组装，码值单一来源）
        // 头文件 KF.hpp 中只放 extern 声明，改码时只需重编译本文件
        /////////////////////////////////////////////////////////

        // 通用 / 测试模块 (01)
        const Code TEST_INFO  = MakeCode(Module::Common, LogLevel::Info,    0x01, 0x001);
        const Code TEST_WARN  = MakeCode(Module::Common, LogLevel::Warning, 0x01, 0x002);
        const Code TEST_ERROR = MakeCode(Module::Common, LogLevel::Error,   0x01, 0x003);
        const Code TEST_FATAL = MakeCode(Module::Common, LogLevel::Fatal,   0x01, 0x004);
        const Code SYSTEM_OOM = MakeCode(Module::Common, LogLevel::Fatal,   0x01, 0x005);

        // KFIO 模块 (02)
        const Code KFIO_FILE_OPEN_FAIL = MakeCode(Module::KFIO, LogLevel::Fatal, 0x01, 0x001);
        const Code KFIO_FILE_READ_FAIL = MakeCode(Module::KFIO, LogLevel::Fatal, 0x01, 0x002);

        // KCLI 模块 (05)
        const Code KCLI_INPUT_INVALID = MakeCode(Module::KCLI, LogLevel::Warning, 0x01, 0x001);

        // KTIMER 模块 (04)
        const Code KTIMER_NOT_FOUND       = MakeCode(Module::KTIMER, LogLevel::Warning, 0x01, 0x001);
        const Code KTIMER_ALREADY_EXISTS  = MakeCode(Module::KTIMER, LogLevel::Warning, 0x01, 0x002);
        const Code KTIMER_STATE_ERROR     = MakeCode(Module::KTIMER, LogLevel::Warning, 0x01, 0x003);

        // KSON 模块 (03)
        const Code KSON_PARSE_STRE             = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x001);
        const Code KSON_PARSE_STR_NOEND        = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x002);
        const Code KSON_PARSE_MULPOINT         = MakeCode(Module::KSON, LogLevel::Warning, 0x01, 0x003);
        const Code KSON_PARSE_NUM_UE           = MakeCode(Module::KSON, LogLevel::Warning, 0x01, 0x004);
        const Code KSON_PARSE_NUMOR            = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x005);
        const Code KSON_PARSE_NUM_USTYPE       = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x006);
        const Code KSON_PARSE_ESCAPE_SPECIAL   = MakeCode(Module::KSON, LogLevel::Warning, 0x01, 0x007);
        const Code KSON_PARSE_UNFINISHED_ESCAPE= MakeCode(Module::KSON, LogLevel::Fatal,   0x01, 0x00A);
        const Code KSON_PARSE_BIG_EXP          = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x00C);
        const Code KSON_PARSE_VAL_END          = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0AA);
        const Code KSON_PARSE_VAL_ERROR        = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0AC);
        const Code KSON_PARSE_ARR_BEGIN        = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0AE);
        const Code KSON_PARSE_ARRUE            = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0B0);
        const Code KSON_PARSE_OBJ_BEGIN        = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0B2);
        const Code KSON_PARSE_OBJ_KEY_QUOTE    = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0B4);
        const Code KSON_PARSE_OBJ_SEPERATOR    = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0B6);
        const Code KSON_PARSE_OBJUE            = MakeCode(Module::KSON, LogLevel::Error,   0x01, 0x0B8);
        const Code KSON_PARSE_TRAIL            = MakeCode(Module::KSON, LogLevel::Warning, 0x02, 0x011);
        const Code KSON_TYPE_MISMATCH          = MakeCode(Module::KSON, LogLevel::Fatal,   0x01, 0x001);
        // KMATH 模块 (06)
        const Code KMATH_MULPOINT            = MakeCode(Module::KMATH, LogLevel::Warning, 0x01, 0x002);
        const Code KMATH_INVALIDCHAR         = MakeCode(Module::KMATH, LogLevel::Warning, 0x01, 0x004);
        const Code KMATH_DIVBYZERO           = MakeCode(Module::KMATH, LogLevel::Error, 0x02, 0x003);
        // 未知模块 (00)
        const Code UNKNOWN = MakeCode(Module::Unknown, LogLevel::Fatal, 0x00, 0x000);

        /// @brief 错误码 -> 说明文本 码表
        /// @note  键统一使用本文件中定义的 const Code 常量，码值单一来源，
        ///        改码时只需改本文件，头文件 extern 声明自动同步，避免硬编码 hex 失配
        std::unordered_map<Code, std::string_view> Table =
        {
            {UNKNOWN,                       "Unknown,try to look the msg"},
            // 通用 / 测试模块
            {TEST_INFO,                     "测试-信息"},
            {TEST_WARN,                     "测试-警告"},
            {TEST_ERROR,                    "测试-错误"},
            {TEST_FATAL,                    "测试-严重错误"},
            {SYSTEM_OOM,                    "out of memory,can not allocate memory"},
            // KFIO 模块
            {KFIO_FILE_OPEN_FAIL,           "KFIO open file failed"},
            {KFIO_FILE_READ_FAIL,           "KFIO read file failed"},
            // KCLI 模块
            {KCLI_INPUT_INVALID,            "KCLI input invalid, failed to parse"},
            // KTIMER 模块
            {KTIMER_NOT_FOUND,              "KTIMER timer not found"},
            {KTIMER_ALREADY_EXISTS,         "KTIMER timer already exists, will be overwritten"},
            {KTIMER_STATE_ERROR,            "KTIMER timer state does not allow this operation"},
            // KSON 模块
            {KSON_PARSE_STRE,               "KSON Parse string, expecting a quote"},
            {KSON_PARSE_STR_NOEND,          "KSON Parse string, expecting an ending quote"},
            {KSON_PARSE_MULPOINT,           "KSON Parse number, multiple decimal points"},
            {KSON_PARSE_NUM_UE,             "KSON Parse number, The number contains unsupported non-Arabic digits"},
            {KSON_PARSE_NUMOR,               "KSON Parse number, Can't store this number, maybe it's out of range"},
            {KSON_PARSE_NUM_USTYPE,         "KSON Parse number, return an unsupported number type"},
            {KSON_PARSE_ESCAPE_SPECIAL,     "KSON Parse string, There's an invalid char following the escape char"},
            {KSON_PARSE_UNFINISHED_ESCAPE,  "KSON Parse string, out of range when reading an escape"},
            {KSON_PARSE_BIG_EXP,            "KSON Parse number, exponent too large for scientific notation"},
            {KSON_PARSE_VAL_END,            "KSON Parse value, the fileRead is ended"},
            {KSON_PARSE_VAL_ERROR,          "KSON Parse value, error occured"},
            {KSON_PARSE_ARR_BEGIN,          "KSON Parse array, expecting a opening bracket"},
            {KSON_PARSE_ARRUE,              "KSON Parse array, unexpected char"},
            {KSON_PARSE_OBJ_BEGIN,          "KSON Parse object, expecting a opening brace"},
            {KSON_PARSE_OBJ_KEY_QUOTE,      "KSON Parse object, expected a quote"},
            {KSON_PARSE_OBJ_SEPERATOR,      "KSON Parse object, expected a colon separator"},
            {KSON_PARSE_OBJUE,              "KSON Parse object, unexpected char"},
            {KSON_PARSE_TRAIL,              "KSON Parse string, unexpected following char"},
            {KSON_TYPE_MISMATCH,            "KSON AsSth type mismatch"},
            {KMATH_MULPOINT,              "KMATH Parse number, multiple decimal points"},
        {KMATH_INVALIDCHAR,           "KMATH Parse number, The number contains unsupported non-Arabic digits"},
        {KMATH_DIVBYZERO,             "KMATH divide by zero, result is +/-inf (0/0 is nan)"},
        };
    };
}

/**
 * @brief 错误码编码说明（与 KF.hpp 中 KF::KLOGGER 命名空间内的 MakeCode / LogLevel / Module 对应）
 *
 * 1. 错误码格式：0x[aa][b][cc][ddd]，共 8 位 16 进制
 *      - [aa]：模块号（2 位 16 进制，bit24-31）
 *      - [b] ：等级  （1 位 16 进制，bit20-23）
 *      - [cc]：类型  （2 位 16 进制，bit12-19）
 *      - [ddd]：序号 （3 位 16 进制，bit0-11）
 *
 * 2. 模块号（见 KF::KLOGGER::Module）
 *      - 00：未知模块
 *      - 01：通用模块（测试用）
 *      - 02：KFIO
 *      - 03：KSON
 *      - 04：KTIMER
 *
 * 3. 等级（见 KF::KLOGGER::LogLevel）
 *      - 1：信息    Info
 *      - 2：警告    Warning
 *      - 3：错误    Error
 *      - 4：严重错误 Fatal
 *
 * 4. 组装方式：统一用 MakeCode(module, level, type, id)，
 *    切勿手写 hex，避免码表与常量失配。
 *
 * 5. 调用方式：用宏 KLOG_ERROR / KLOG_WARNING / KLOG_INFO / KLOG_FATAL
 *    自动捕获 __FILE__ / __LINE__ / __FUNCTION__，
 *    勿直接调用 KF::KLOGGER::Error 等函数（缺少位置信息）。
 *
 * @note LogLevel / Module / MakeCode 留在头文件（constexpr 基础设施），
 *       错误码常量的 extern 声明在头文件，定义在本文件（KLOGGER.cpp），
 *       改码时只需重编译本文件。KF.hpp 末尾通过 using namespace KF::KLOGGER
 *       将其引入全局作用域，调用处无需前缀：KLOG_ERROR(KSON_PARSE_STRE, "")
**/
