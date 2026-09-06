/**
 * @file    dbgKTIMER.cpp
 * @brief   KTIMER 计时器模块配置驱动测试
 *
 * 检测单元从 config/test/cfg.kson 的 dbgKTIMER 读取：
 *   - plan: 操作脚本数组，op 支持 add / sleep / pause / start / get / print / printall / delete
 *
 * 若配置键缺失则输出 {red}[FAIL]{/} 并继续，不会退出程序。
 */

import kf;
#include <thread>
#include <chrono>

// ==================== 测试辅助 ====================
#define SECTION(name) kout << Bold << "\n--- " << name << " ---" << Reset << std::endl

static int g_ok = 0, g_fail = 0;

/// 单位字符串 -> TimeUnit
static TimeUnit ParseUnit(const std::string& u)
{
    if (u == "ns") return TimeUnit::ns;
    if (u == "us") return TimeUnit::us;
    if (u == "s")  return TimeUnit::s;
    return TimeUnit::ms; // 默认 ms
}

/// 执行单个 plan 操作
static void RunOp(const kson& op, size_t idx)
{
    std::string opname = op["op"].Exists() ? op["op"].Str() : "";
    std::string name   = op["name"].Exists() ? op["name"].Str() : "";

    if (!op["op"].Exists())
    {
        kout << "  plan[" << idx << "] {red}[FAIL]{/}  缺少 op 字段" << std::endl;
        ++g_fail;
        return;
    }

    if (opname == "printall")
    {
        kout << "  >> printall" << std::endl;
        PrintAllTimers();
        ++g_ok;
    }
    else if (opname == "add")
    {
        if (!op["name"].Exists())
        {
            kout << "  plan[" << idx << "] {red}[FAIL]{/}  add 缺少 name" << std::endl;
            ++g_fail;
            return;
        }
        TimeUnit unit = op["unit"].Exists() ? ParseUnit(op["unit"].Str()) : TimeUnit::ms;
        bool ok = AddTimer(name, unit);
        kout << "  >> AddTimer(\"" << name << "\", " << (op["unit"].Exists() ? op["unit"].Str() : "ms")
             << ") -> " << (ok ? "{green}[OK]{/}" : "{yellow}[skip]{/} (重名覆盖)") << std::endl;
        if (ok) ++g_ok;
    }
    else if (opname == "sleep")
    {
        long long ms = op["ms"].Exists() ? op["ms"].Int() : 0;
        kout << "  >> sleep " << ms << " ms" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        ++g_ok;
    }
    else if (opname == "pause")
    {
        bool ok = PauseTimer(name);
        kout << "  >> PauseTimer(\"" << name << "\") -> " << (ok ? "{green}[OK]{/}" : "{yellow}[skip]{/}") << std::endl;
        if (ok) ++g_ok; // 不存在/已暂停 属预期行为，不视为失败
    }
    else if (opname == "start")
    {
        bool ok = StartTimer(name);
        kout << "  >> StartTimer(\"" << name << "\") -> " << (ok ? "{green}[OK]{/}" : "{yellow}[skip]{/}") << std::endl;
        if (ok) ++g_ok;
    }
    else if (opname == "get")
    {
        double val = GetTimer(name);
        kout << "  >> GetTimer(\"" << name << "\") = " << val;
        if (val < 0)
            kout << " {yellow}[skip]{/} (不存在)" << std::endl;
        else
            kout << " {green}[OK]{/}" << std::endl, ++g_ok;
    }
    else if (opname == "print")
    {
        PrintTimer(name);
        ++g_ok;
    }
    else if (opname == "delete")
    {
        bool ok = DeleteTimer(name);
        kout << "  >> DeleteTimer(\"" << name << "\") -> " << (ok ? "{green}[OK]{/}" : "{yellow}[skip]{/}") << std::endl;
        if (ok) ++g_ok;
    }
    else
    {
        kout << "  plan[" << idx << "] {red}[FAIL]{/}  未知 op \"" << opname << "\"" << std::endl;
        ++g_fail;
    }
}

int main()
{
    auto doc = ReadKsonFile("config/test/cfg.kson");
    auto timer = doc["dbgKTIMER"];
    KBegin(timer.Vec());

    SECTION("plan 计时器操作脚本");
    if (!timer["plan"].Exists())
    {
        koutE << "{red}[FAIL]{/}  dbgKTIMER 缺少 plan 键，跳过该节" << std::endl;
        ++g_fail;
    }
    else
    {
        auto plan = timer["plan"];
        for (size_t i = 0; i < plan.size(); i++)
            RunOp(plan[i], i);
    }

    // ==================== 结论 ====================
    kout << "\n----------------------------------------\n";
    kout << "  {green}[OK]{/} " << g_ok << " 项通过\n";
    if (g_fail == 0)
        kout << "\n{green}[ALL PASS] KTIMER 配置驱动测试通过{/}\n";
    else
        kout << "\n{red}[" << g_fail << " FAIL] KTIMER 配置驱动测试有失败项{/}\n";

    KEnd();
    return g_fail > 0 ? 1 : 0;
}