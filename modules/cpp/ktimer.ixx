module;
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

// 日志宏：自动捕获调用位置（本模块内使用，位于  内）
#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module ktimer;

import klogger;

export
{
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

    bool AddTimer(const std::string& name, TimeUnit unit);
    bool PauseTimer(const std::string& name);
    bool StartTimer(const std::string& name);
    bool DeleteTimer(const std::string& name);
    double GetTimer(const std::string& name);
    void PrintTimer(const std::string& name);
    void PrintAllTimers();
}



    struct TimerEntry
    {
        TimeUnit unit = TimeUnit::ms;
        TimerState state = TimerState::Paused;
        std::chrono::steady_clock::time_point start_point{};
        std::chrono::nanoseconds accumulated{0};

        std::chrono::nanoseconds TotalNs() const
        {
            if (state == TimerState::Running)
                return accumulated + std::chrono::steady_clock::now() - start_point;
            return accumulated;
        }

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

    static std::unordered_map<std::string, TimerEntry>& Timers()
    {
        static std::unordered_map<std::string, TimerEntry> table;
        return table;
    }

    static size_t DisplayWidth(std::string_view s)
    {
        size_t width = 0;
        for (size_t i = 0; i < s.size(); )
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if      (c < 0x80) { width += 1; i += 1; }
            else if (c < 0xC0) { i += 1; }
            else if (c < 0xE0) { width += 1; i += 2; }
            else if (c < 0xF0) { width += 2; i += 3; }
            else               { width += 2; i += 4; }
        }
        return width;
    }

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

    static const char* StateStr(TimerState s)
    {
        switch (s)
        {
            case TimerState::Running: return "Running";
            case TimerState::Paused:  return "Paused";
        }
        return "?";
    }

    static const char* StateColor(TimerState s)
    {
        return (s == TimerState::Running) ? Green : LightYellow;
    }

    static std::string FormatTime(double val, TimeUnit unit)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << val << " " << UnitStr(unit);
        return oss.str();
    }

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

        std::string titleLine   = "  Timer: " + name;
        std::string stateLine   = "  State   : " + stateStr;
        std::string elapsedLine = "  Elapsed : " + elapsedStr;

        size_t maxW = DisplayWidth(titleLine);
        if (DisplayWidth(stateLine)   > maxW) maxW = DisplayWidth(stateLine);
        if (DisplayWidth(elapsedLine) > maxW) maxW = DisplayWidth(elapsedLine);

        size_t innerW = maxW + 2;
        std::string bar(innerW, '-');

        const char* F = SkyBlue;
        const char* B = Bold;
        const char* R = Reset;

        std::cout << "\n" << F << "+" << bar << "+\n";

        {
            size_t pad = innerW - DisplayWidth(titleLine);
            std::cout << F << "|" << B << titleLine << R << F
                      << std::string(pad, ' ') << "|\n";
        }
        std::cout << F << "+" << bar << "+\n";

        {
            size_t pad = innerW - DisplayWidth(stateLine);
            const char* sc = StateColor(t.state);
            std::cout << F << "|  State   : " << sc << stateStr << R << F
                      << std::string(pad, ' ') << "|\n";
        }

        {
            size_t pad = innerW - DisplayWidth(elapsedLine);
            std::cout << F << "|  Elapsed : " << B << elapsedStr << R << F
                      << std::string(pad, ' ') << "|\n";
        }

        std::cout << F << "+" << bar << "+" << R << std::endl;
    }

    void PrintAllTimers()
    {
        auto& timers = Timers();

        if (timers.empty())
        {
            std::string msg = "  KTIMER - 无计时器";
            size_t w = DisplayWidth(msg);
            size_t innerW = w + 2;
            std::string bar(innerW, '-');
            std::cout << "\n" << SkyBlue << "+" << bar << "+\n"
                      << "|" << Bold << msg << Reset << SkyBlue
                      << std::string(innerW - w, ' ') << "|\n"
                      << "+" << bar << "+" << Reset << std::endl;
            return;
        }

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

        const char* hName    = "Name";
        const char* hState   = "State";
        const char* hElapsed = "Elapsed";

        size_t wName    = DisplayWidth(hName);
        size_t wState   = DisplayWidth(hState);
        size_t wElapsed = DisplayWidth(hElapsed);
        for (auto& r : rows)
        {
            if (DisplayWidth(r.name)    > wName)    wName    = DisplayWidth(r.name);
            if (DisplayWidth(r.state)   > wState)   wState   = DisplayWidth(r.state);
            if (DisplayWidth(r.elapsed) > wElapsed) wElapsed = DisplayWidth(r.elapsed);
        }

        const size_t gap = 2;
        size_t contentW = gap + wName + gap + wState + gap + wElapsed;

        std::string title = "  KTIMER - 所有计时器 (" + std::to_string(rows.size()) + ")";
        size_t titleW = DisplayWidth(title);
        size_t innerW = (contentW > titleW ? contentW : titleW) + 2;
        std::string bar(innerW, '-');

        const char* F = SkyBlue;
        const char* B = Bold;
        const char* R = Reset;

        std::cout << "\n" << F << "+" << bar << "+\n";

        {
            size_t pad = innerW - titleW;
            std::cout << F << "|" << B << title << R << F
                      << std::string(pad, ' ') << "|\n";
        }
        std::cout << F << "+" << bar << "+\n";

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
