#include "modules/cpp/KF.hpp"
using namespace KLOG;

/**
 * @file KTIMER.cpp
 * @brief KTIMER 计时器模块实现
 * @version 1.0.0
 * @date 2026-08-13
 * @author Git-1145
 *
 * 功能：
 *   - AddTimer(name, unit)    新建计时器（指定名字和单位），创建后立即开始计时
 *   - PauseTimer(name)        暂停计时器（累计已运行时间）
 *   - StartTimer(name)        恢复已暂停的计时器
 *   - DeleteTimer(name)       删除计时器
 *   - GetTimer(name)          获取计时器当前累计时间（double）
 *   - PrintTimer(name)        打印单个计时器（格式化框）
 *   - PrintAllTimers()        打印所有计时器（格式化表格）
 *
 * 与其他模块的结合：
 *   - KLOGGER：错误码声明在 KF.hpp，定义在 KLOGGER.cpp，通过 KLOG_* 宏上报
 *   - Color：复用 KF::KLOGGER::Color 的 VT100 颜色常量
 *   - KCLI：可与 KBegin/kout/kpause 等配合使用，打印风格统一（SkyBlue 框线）
 */

namespace KF
{
    namespace KTIMER
    {
        // Color 别名（与 KCLI 一致，定义在 KF::KLOGGER::Color）
        namespace Color = KF::KLOGGER::Color;

        /////////////////////////////////////////////////////////
        // 内部数据结构
        /////////////////////////////////////////////////////////

        /// @brief 计时器条目
        struct TimerEntry
        {
            TimeUnit unit = TimeUnit::ms;
            TimerState state = TimerState::Paused;
            std::chrono::steady_clock::time_point start_point{};
            std::chrono::nanoseconds accumulated{0};  ///< 暂停时累计的纳秒数

            /// @brief 获取当前累计时间（纳秒），运行中则加上当前周期
            std::chrono::nanoseconds TotalNs() const
            {
                if (state == TimerState::Running)
                    return accumulated + std::chrono::steady_clock::now() - start_point;
                return accumulated;
            }

            /// @brief 按计时器单位返回时间值
            double Elapsed() const
            {
                auto ns = TotalNs().count();
                switch (unit)
                {
                    case TimeUnit::ns: return static_cast<double>(ns);
                    case TimeUnit::us: return ns / 1e3;
                    case TimeUnit::ms: return ns / 1e6;
                    case TimeUnit::s:  return ns / 1e9;
                }
                return 0.0;
            }
        };

        /// @brief 计时器表（名称 -> 条目），函数内 static 保证初始化顺序
        static std::unordered_map<std::string, TimerEntry>& Timers()
        {
            static std::unordered_map<std::string, TimerEntry> table;
            return table;
        }

        /////////////////////////////////////////////////////////
        // 内部工具函数
        /////////////////////////////////////////////////////////

        /// @brief 计算 UTF-8 字符串的终端显示宽度（CJK=2, ASCII=1）
        /// @note  用于框线对齐：中文字符占 2 列，ASCII 占 1 列
        static size_t DisplayWidth(std::string_view s)
        {
            size_t width = 0;
            for (size_t i = 0; i < s.size(); )
            {
                unsigned char c = static_cast<unsigned char>(s[i]);
                if      (c < 0x80) { width += 1; i += 1; }  // ASCII
                else if (c < 0xC0) { i += 1; }               // continuation byte（跳过）
                else if (c < 0xE0) { width += 1; i += 2; }  // 2-byte（拉丁扩展，窄）
                else if (c < 0xF0) { width += 2; i += 3; }  // 3-byte（CJK，宽）
                else               { width += 2; i += 4; }  // 4-byte（emoji，宽）
            }
            return width;
        }

        /// @brief 时间单位 -> 字符串
        static const char* UnitStr(TimeUnit u)
        {
            switch (u)
            {
                case TimeUnit::ns: return "ns";
                case TimeUnit::us: return "us";
                case TimeUnit::ms: return "ms";
                case TimeUnit::s:  return "s";
            }
            return "?";
        }

        /// @brief 计时器状态 -> 字符串
        static const char* StateStr(TimerState s)
        {
            switch (s)
            {
                case TimerState::Running: return "Running";
                case TimerState::Paused:  return "Paused";
            }
            return "?";
        }

        /// @brief 状态 -> 颜色（Running=绿, Paused=淡黄）
        static const char* StateColor(TimerState s)
        {
            return (s == TimerState::Running) ? Color::Green : Color::LightYellow;
        }

        /// @brief 格式化时间值字符串（保留 2 位小数 + 单位）
        static std::string FormatTime(double val, TimeUnit unit)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << val << " " << UnitStr(unit);
            return oss.str();
        }

        /////////////////////////////////////////////////////////
        // 计时器功能函数实现
        /////////////////////////////////////////////////////////

        /// @brief 新建计时器（指定名字和单位），创建后立即开始计时
        /// @param name  计时器名称（唯一标识）
        /// @param unit  时间单位（ns/us/ms/s）
        /// @return true=新建成功，false=同名已存在（已覆盖，发出警告）
        bool AddTimer(const std::string& name, TimeUnit unit)
        {
            bool existed = Timers().count(name) > 0;
            if (existed)
                KLOG_WARNING(KTIMER_ALREADY_EXISTS, name);

            TimerEntry t;
            t.unit = unit;
            t.state = TimerState::Running;
            t.start_point = std::chrono::steady_clock::now();
            t.accumulated = std::chrono::nanoseconds(0);
            Timers()[name] = std::move(t);
            return !existed;
        }

        /// @brief 暂停计时器（累计已运行时间）
        /// @return true=暂停成功，false=不存在或未在运行
        bool PauseTimer(const std::string& name)
        {
            auto it = Timers().find(name);
            if (it == Timers().end())
            {
                KLOG_WARNING(KTIMER_NOT_FOUND, name);
                return false;
            }
            if (it->second.state != TimerState::Running)
            {
                KLOG_WARNING(KTIMER_STATE_ERROR, name + " is not running");
                return false;
            }
            it->second.accumulated += std::chrono::steady_clock::now() - it->second.start_point;
            it->second.state = TimerState::Paused;
            return true;
        }

        /// @brief 恢复已暂停的计时器
        /// @return true=恢复成功，false=不存在或未暂停
        bool StartTimer(const std::string& name)
        {
            auto it = Timers().find(name);
            if (it == Timers().end())
            {
                KLOG_WARNING(KTIMER_NOT_FOUND, name);
                return false;
            }
            if (it->second.state != TimerState::Paused)
            {
                KLOG_WARNING(KTIMER_STATE_ERROR, name + " is not paused");
                return false;
            }
            it->second.start_point = std::chrono::steady_clock::now();
            it->second.state = TimerState::Running;
            return true;
        }

        /// @brief 删除计时器
        /// @return true=删除成功，false=不存在
        bool DeleteTimer(const std::string& name)
        {
            auto it = Timers().find(name);
            if (it == Timers().end())
            {
                KLOG_WARNING(KTIMER_NOT_FOUND, name);
                return false;
            }
            Timers().erase(it);
            return true;
        }

        /// @brief 获取计时器当前累计时间（按计时器单位）
        /// @return >=0 累计时间，-1.0 表示不存在
        double GetTimer(const std::string& name)
        {
            auto it = Timers().find(name);
            if (it == Timers().end())
            {
                KLOG_WARNING(KTIMER_NOT_FOUND, name);
                return -1.0;
            }
            return it->second.Elapsed();
        }

        /// @brief 打印单个计时器信息（格式化框）
        void PrintTimer(const std::string& name)
        {
            auto it = Timers().find(name);
            if (it == Timers().end())
            {
                KLOG_WARNING(KTIMER_NOT_FOUND, name);
                return;
            }

            const TimerEntry& t = it->second;
            std::string stateStr   = StateStr(t.state);
            std::string elapsedStr = FormatTime(t.Elapsed(), t.unit);

            // 构建各行纯文本内容（用于计算显示宽度，不含颜色码）
            std::string titleLine   = "  Timer: " + name;
            std::string stateLine   = "  State   : " + stateStr;
            std::string elapsedLine = "  Elapsed : " + elapsedStr;

            size_t maxW = DisplayWidth(titleLine);
            if (DisplayWidth(stateLine)   > maxW) maxW = DisplayWidth(stateLine);
            if (DisplayWidth(elapsedLine) > maxW) maxW = DisplayWidth(elapsedLine);

            size_t innerW = maxW + 2;  // 右侧额外填充
            std::string bar(innerW, '-');

            const char* F = Color::SkyBlue;   // Frame 框线色
            const char* B = Color::Bold;       // Bold 加粗
            const char* R = Color::Reset;      // Reset 重置

            std::cout << "\n" << F << "+" << bar << "+\n";

            // 标题行
            {
                size_t pad = innerW - DisplayWidth(titleLine);
                std::cout << F << "|" << B << titleLine << R << F
                          << std::string(pad, ' ') << "|\n";
            }
            std::cout << F << "+" << bar << "+\n";

            // 状态行
            {
                size_t pad = innerW - DisplayWidth(stateLine);
                const char* sc = StateColor(t.state);
                std::cout << F << "|  State   : " << sc << stateStr << R << F
                          << std::string(pad, ' ') << "|\n";
            }

            // 时间行
            {
                size_t pad = innerW - DisplayWidth(elapsedLine);
                std::cout << F << "|  Elapsed : " << B << elapsedStr << R << F
                          << std::string(pad, ' ') << "|\n";
            }

            std::cout << F << "+" << bar << "+" << R << std::endl;
        }

        /// @brief 打印所有计时器信息（格式化表格，按名称排序）
        void PrintAllTimers()
        {
            auto& timers = Timers();

            // 空表提示
            if (timers.empty())
            {
                std::string msg = "  KTIMER - 无计时器";
                size_t w = DisplayWidth(msg);
                size_t innerW = w + 2;
                std::string bar(innerW, '-');
                std::cout << "\n" << Color::SkyBlue << "+" << bar << "+\n"
                          << "|" << Color::Bold << msg << Color::Reset << Color::SkyBlue
                          << std::string(innerW - w, ' ') << "|\n"
                          << "+" << bar << "+" << Color::Reset << std::endl;
                return;
            }

            // 收集行数据并按名称排序
            struct Row
            {
                std::string name;
                std::string state;
                std::string elapsed;
                TimerState  stateEnum;
            };
            std::vector<Row> rows;
            rows.reserve(timers.size());
            for (auto& [key, t] : timers)
            {
                rows.push_back({key, StateStr(t.state),
                                FormatTime(t.Elapsed(), t.unit), t.state});
            }
            std::sort(rows.begin(), rows.end(),
                      [](const Row& a, const Row& b) { return a.name < b.name; });

            // 列标题
            const char* hName    = "Name";
            const char* hState   = "State";
            const char* hElapsed = "Elapsed";

            // 计算列宽（纯数据宽度，不含间距）
            size_t wName    = DisplayWidth(hName);
            size_t wState   = DisplayWidth(hState);
            size_t wElapsed = DisplayWidth(hElapsed);
            for (auto& r : rows)
            {
                if (DisplayWidth(r.name)    > wName)    wName    = DisplayWidth(r.name);
                if (DisplayWidth(r.state)   > wState)   wState   = DisplayWidth(r.state);
                if (DisplayWidth(r.elapsed) > wElapsed) wElapsed = DisplayWidth(r.elapsed);
            }

            // 列间距：首列前 2 空格，列间 2 空格
            const size_t gap = 2;
            size_t contentW = gap + wName + gap + wState + gap + wElapsed;

            // 标题
            std::string title = "  KTIMER - 所有计时器 (" + std::to_string(rows.size()) + ")";
            size_t titleW = DisplayWidth(title);
            size_t innerW = (contentW > titleW ? contentW : titleW) + 2;
            std::string bar(innerW, '-');

            const char* F = Color::SkyBlue;
            const char* B = Color::Bold;
            const char* R = Color::Reset;

            std::cout << "\n" << F << "+" << bar << "+\n";

            // 标题行
            {
                size_t pad = innerW - titleW;
                std::cout << F << "|" << B << title << R << F
                          << std::string(pad, ' ') << "|\n";
            }
            std::cout << F << "+" << bar << "+\n";

            // 表头行
            {
                std::cout << F << "|" << B
                          << std::string(gap, ' ')
                          << hName    << std::string(wName    - DisplayWidth(hName), ' ')
                          << std::string(gap, ' ')
                          << hState   << std::string(wState   - DisplayWidth(hState), ' ')
                          << std::string(gap, ' ')
                          << hElapsed << std::string(wElapsed - DisplayWidth(hElapsed), ' ')
                          << R << F
                          << std::string(innerW - contentW, ' ')
                          << "|\n";
            }
            std::cout << F << "+" << bar << "+\n";

            // 数据行
            for (auto& r : rows)
            {
                const char* sc = StateColor(r.stateEnum);
                std::cout << F << "|" << R
                          << std::string(gap, ' ')
                          << r.name    << std::string(wName    - DisplayWidth(r.name), ' ')
                          << std::string(gap, ' ')
                          << sc << r.state << R
                          << std::string(wState - DisplayWidth(r.state), ' ')
                          << std::string(gap, ' ')
                          << B << r.elapsed << R << F
                          << std::string(wElapsed - DisplayWidth(r.elapsed), ' ')
                          << std::string(innerW - contentW, ' ')
                          << "|\n";
            }

            std::cout << F << "+" << bar << "+" << R << std::endl;
        }
    }
}
