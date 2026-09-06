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
}
