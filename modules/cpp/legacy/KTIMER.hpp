#pragma once
#include "KFCommon.hpp"
#include "KLOGGER.hpp"
namespace KF
{
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
}
