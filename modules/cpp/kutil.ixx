module;
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <windows.h>

export module kutil;

import klogger;
import kson;

export
{
    /////////////////////////////////////////////////////////
    // 可视化输出（自 kcli 迁入）
    /////////////////////////////////////////////////////////

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

    /// @brief 检查/调整控制台尺寸以容纳 rows 行 x cols 列（完全静默）
    bool CheckConsoleFit(int rows, int cols, bool keepIfFits = false);

    /// @brief 清屏
    void ClearScreen();

    /// @brief 恢复控制台字体（程序退出前调用）
    void RestoreConsoleFont();
}

    /////////////////////////////////////////////////////////
    // 迷宫字符常量（与 kson::MazeCell 枚举序对应；PATH 用 'L'）
    /////////////////////////////////////////////////////////
    constexpr char Wall     = 'W';
    constexpr char Passable = 'P';
    constexpr char Visited  = 'V';
    constexpr char Start    = 'S';
    constexpr char End      = 'E';
    constexpr char Path     = 'L';

    void ClearScreen()
    {
        std::cout << "\033[?25h\033[H\033[J" << std::flush;
    }

    void Arr::Print(const std::vector<size_t>& ranks, size_t n, int barMax, int highlight1, int highlight2, int sortedUntil)
    {
        std::string out;
        out.reserve(n * (barMax + 32));
        out += "\033[H";
        const char* last = nullptr;
        for (size_t i = 0; i < n; i++)
        {
            const char* color;
            if (i == (size_t)highlight1)      color = "\033[1;32m";
            else if (i == (size_t)highlight2) color = "\033[1;33m";
            else if (sortedUntil >= 0 && (int)i > sortedUntil) color = "\033[90m";
            else                              color = "\033[0m";
            if (color != last) { out += color; last = color; }

            int barLen = (int)(ranks[i] * barMax / n);
            if (barLen < 0) barLen = 0;
            if (barLen > barMax) barLen = barMax;

            out.append((size_t)barLen, '#');
            out += "\033[K\n";
        }
        out += "\033[0m";
        std::cout << out << std::flush;
    }

    void Maze::Print(const std::vector<std::vector<MazeCell>>& maze)
    {
        static const std::string cellColor[] = {
            Gray,                              // WALL
            LightGray,                         // PASSABLE
            Yellow,                            // VISITED
            std::string(Bold) + Green,  // START
            std::string(Bold) + Blue,   // END
            std::string(Bold) + Blue,   // PATH
        };
        static const char cellChar[] = { Wall, Passable, Visited, Start, End, Path };

        std::string out;
        out.reserve(maze.size() * (maze[0].size() + 8));
        out += "\033[H";
        int last = -1;
        for (const auto& row : maze)
        {
            for (auto cell : row)
            {
                int id = (int)cell;
                if (id != last) { out += cellColor[id]; last = id; }
                out += cellChar[id];
            }
            out += "\033[K\n";
        }
        out += "\033[0m";
        std::cout << out << std::flush;
    }

    /////////////////////////////////////////////////////////
    // 控制台适配：保存/恢复字体、调整窗口尺寸
    /////////////////////////////////////////////////////////
    static CONSOLE_FONT_INFOEX g_savedFont{};
    static bool g_fontSaved = false;
    static void SaveOriginalFont()
    {
        if (g_fontSaved) return;
        CONSOLE_FONT_INFOEX cfi{};
        cfi.cbSize = sizeof(cfi);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (GetCurrentConsoleFontEx(h, FALSE, &cfi)) { g_savedFont = cfi; g_fontSaved = true; }
    }
    void RestoreConsoleFont()
    {
        if (!g_fontSaved) return;
        SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &g_savedFont);
        Sleep(100);
    }

    static void FitConsoleClassic(HANDLE h, int rows, int cols)
    {
        CONSOLE_FONT_INFOEX cfi{};
        cfi.cbSize = sizeof(cfi);
        SaveOriginalFont();
        short fontY = GetCurrentConsoleFontEx(h, FALSE, &cfi) ? cfi.dwFontSize.Y : 6;

        while (true)
        {
            COORD max = GetLargestConsoleWindowSize(h);
            int wc = cols + 1 < (int)max.X ? cols + 1 : (int)max.X;
            int wr = rows + 2 < (int)max.Y ? rows + 2 : (int)max.Y;

            SMALL_RECT one = {0, 0, 0, 0};
            SetConsoleWindowInfo(h, TRUE, &one);
            SetConsoleScreenBufferSize(h, {(SHORT)wc, (SHORT)wr});
            SMALL_RECT win = {0, 0, (SHORT)(wc - 1), (SHORT)(wr - 1)};
            SetConsoleWindowInfo(h, TRUE, &win);
            Sleep(60);

            if (wr >= rows + 2 && wc >= cols + 1) return;
            if (fontY <= 6) break;

            fontY = fontY - 2 < 6 ? 6 : (short)(fontY - 2);
            cfi.dwFontSize.X = 0;
            cfi.dwFontSize.Y = fontY;
            SetCurrentConsoleFontEx(h, FALSE, &cfi);
            Sleep(150);
        }

        COORD max = GetLargestConsoleWindowSize(h);
        int wc = (int)max.X, wr = (int)max.Y;
        SMALL_RECT one = {0, 0, 0, 0};
        SetConsoleWindowInfo(h, TRUE, &one);
        SetConsoleScreenBufferSize(h, {(SHORT)wc, (SHORT)wr});
        SMALL_RECT win = {0, 0, (SHORT)(wc - 1), (SHORT)(wr - 1)};
        SetConsoleWindowInfo(h, TRUE, &win);
        Sleep(60);
    }

    bool CheckConsoleFit(int rows, int cols, bool keepIfFits)
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (keepIfFits)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(h, &csbi))
            {
                int winCols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
                int winRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
                if (cols + 1 <= winCols && rows + 2 <= winRows)
                    return true;
            }
        }
        FitConsoleClassic(h, rows, cols);
        return true;
    }