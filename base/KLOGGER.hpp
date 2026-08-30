#pragma once
#include "KFCommon.hpp"
namespace KF
{
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
            constexpr uint32_t KMATH   = 0x06; // KMATH 大数运算模块
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
        //KMATH 模块 (06)
        extern const Code KMATH_MULPOINT;            // 解析警告 多余的小数点.
        extern const Code KMATH_INVALIDCHAR;        // 解析警告 数字中有不支持的非阿拉伯数字
        extern const Code KMATH_DIVBYZERO;        // 除零错误 ERROR
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
}
