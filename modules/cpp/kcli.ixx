module;
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <cstdlib>
#include <windows.h>

// 日志宏：自动捕获调用位置（本模块内使用，位于  内）
#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module kcli;

import klogger;
import kson;
import kutil;

export
{
    /////////////////////////////////////////////////////////
    // KCLI 专属配置键
    /////////////////////////////////////////////////////////
    constexpr const char* FILE_META = "meta";
    constexpr const char* FILE_CMDTITLE = "cmdtitle";
    constexpr const char* FILE_TITLE = "title";
    constexpr const char* FILE_DESC = "description";
    constexpr const char* FILE_AUTHOR = "author";
    constexpr const char* FILE_DATE = "date";
    constexpr const char* FILE_OPTION = "options";
    constexpr const char* FILE_OPTNAME = "name";
    constexpr const char* KBEGIN_UNKNOWN = "Unknown";

    /// @brief 链式输出流，自带默认颜色，支持 {tag} 颜色标签
    class Kout
    {
        const char* color_;
    public:
        explicit constexpr Kout(const char* c) noexcept : color_(c) {}

        static std::string ParseTags(std::string_view str);

        template<typename T>
        Kout& operator<<(const T& val)
        {
            std::cout << color_ << val;
            return *this;
        }
        Kout& operator<<(const std::string& val)
        {
            std::cout << color_ << ParseTags(val);
            return *this;
        }
        Kout& operator<<(const char* val)
        {
            std::cout << color_ << ParseTags(std::string_view(val));
            return *this;
        }
        Kout& operator<<(std::ostream& (*manip)(std::ostream&))
        {
            std::cout << Reset << manip;
            return *this;
        }
    };

    /// @brief 链式输入会话：kin >> a >> b >> c，整组校验，任一非法整组重输
    class KinSession
    {
        std::vector<std::function<bool(const std::string&)>> setters_;
    public:
        KinSession() = default;
        KinSession(const KinSession&) = delete;
        KinSession(KinSession&&) = default;
        KinSession& operator=(const KinSession&) = delete;
        KinSession& operator=(KinSession&&) = default;
        ~KinSession() { commit(); }

        template<typename T>
        KinSession&& operator>>(T& val)
        {
            setters_.push_back(makeSetter(val));
            return std::move(*this);
        }

    private:
        template<typename T>
        static std::function<bool(const std::string&)> makeSetter(T& val)
        {
            if constexpr (std::is_same_v<T, std::size_t>)
            {
                return [&val](const std::string& t) -> bool {
                    try {
                        if (!t.empty() && t[0] == '-') return false;
                        std::size_t idx = 0;
                        const unsigned long long v = std::stoull(t, &idx);
                        if (idx != t.size() || v > (std::numeric_limits<std::size_t>::max)()) return false;
                        val = static_cast<std::size_t>(v);
                        return true;
                    }
                    catch (...) { return false; }
                };
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                return [&val](const std::string& t) -> bool { val = t; return true; };
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return [&val](const std::string& t) -> bool
                {
                    std::string low = t;
                    const std::vector<std::string> trueStrs = {"1", "true", "yes", "y", "on","ture","t"};
                    const std::vector<std::string> falseStrs = {"0", "false", "no", "n", "off","flase","f"};
                    for (auto& c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    if      (std::find(trueStrs.begin(), trueStrs.end(), low) != trueStrs.end()) { val = true;  return true; }
                    else if (std::find(falseStrs.begin(), falseStrs.end(), low) != falseStrs.end()) { val = false; return true; }
                    return false;
                };
            }
            else if constexpr (std::is_integral_v<T>)
            {
                return [&val](const std::string& t) -> bool {
                    try {
                        std::size_t idx = 0;
                        const long long v = std::stoll(t, &idx);
                        if (idx != t.size()) return false;
                        if constexpr (std::is_unsigned_v<T>) { if (v < 0) return false; }
                        val = static_cast<T>(v);
                        return true;
                    }
                    catch (...) { return false; }
                };
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                return [&val](const std::string& t) -> bool {
                    try { val = static_cast<T>(std::stod(t)); return true; }
                    catch (...) { return false; }
                };
            }
            else
            {
                return [&val](const std::string&) -> bool { return false; };
            }
        }

        void commit()
        {
            const std::size_t n = setters_.size();
            while (true)
            {
                std::vector<std::string> tokens;
                tokens.reserve(n);
                while (tokens.size() < n)
                {
                    std::string t;
                    if (!(std::cin >> t)) return;
                    tokens.push_back(t);
                }
                bool allok = true;
                for (std::size_t i = 0; i < n; ++i)
                    if (!setters_[i](tokens[i])) { allok = false; break; }
                if (allok) return;
                Kout(Orange) << "\n输入不合法，请重新输入整组（以空格分隔，共 " << n << " 个值）：" << std::endl;
            }
        }
    };

    /// @brief 链式输入流
    class Kin
    {
    public:
        template<typename T>
        KinSession operator>>(T& val)
        {
            KinSession s;
            s >> val;
            return std::move(s);
        }
    };

    inline Kout kout(Bold);
    inline Kout koutW(LightYellow);
    inline Kout koutE(Orange);
    inline Kout koutF(Red);
    inline Kin kin;

    /// @brief 全局配置（global.kson 的 data 节点）
    extern kson GLOBAL;

    void KBeginImpl(const std::string& cmdtitle, const std::string& title,
                    const std::string& description, const std::string& author,
                    const std::string& date);

    /// @brief 从字符串列表初始化 CLI：{cmdtitle, title, description, author, date}
    void KBegin(const std::vector<std::string>& args);

    /// @brief 显示选项菜单，循环等待用户输入合法选项（返回 0-based 索引）
    std::size_t KOptions(const std::vector<std::string>& options, const std::string& title = "");

    /// @brief 暂停：显示"按任意键继续..."并等待按键
    void KPause();

    /// @brief 结束：暂停后退出程序
    void KEnd();
}



    kson GLOBAL;

    /////////////////////////////////////////////////////////
    // 内部工具函数
    /////////////////////////////////////////////////////////
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

    static void EnableVT100()
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        if (GetConsoleMode(h, &mode))
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    static void SetTitleUTF8(const std::string& title)
    {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return;
        std::wstring wtitle(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wtitle.data(), wlen);
        SetConsoleTitleW(wtitle.c_str());
    }

    static size_t DisplayWidthNoAnsi(std::string_view s)
    {
        std::string clean;
        clean.reserve(s.size());
        for (size_t i = 0; i < s.size(); )
        {
            if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[')
            {
                i += 2;
                while (i < s.size() && !(s[i] >= '0' && s[i] <= '?')) ++i;
                if (i < s.size()) ++i;
                continue;
            }
            clean += s[i];
            ++i;
        }
        return DisplayWidth(clean);
    }

    static const std::unordered_map<std::string_view, const char*>& TagMap()
    {
        static const std::unordered_map<std::string_view, const char*> m = {
            {"red",          Red},
            {"green",        Green},
            {"blue",         Blue},
            {"yellow",       Yellow},
            {"skyblue",      SkyBlue},
            {"orange",       Orange},
            {"magenta",      Magenta},
            {"cyan",         Cyan},
            {"lightyellow",  LightYellow},
            {"lightgray",    LightGray},
            {"bold",         "\033[1m"},
            {"dim",          "\033[2m"},
            {"underline",    "\033[4m"},
            {"blink",        "\033[5m"},
            {"/",            Reset},
            {"/bold",        "\033[22m"},
            {"/dim",         "\033[22m"},
            {"/underline",   "\033[24m"},
            {"/blink",       "\033[25m"},
        };
        return m;
    }

    std::string Kout::ParseTags(std::string_view str)
    {
        std::string out;
        out.reserve(str.size());
        auto& map = TagMap();
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            if (str[i] == '\t') continue;
            if (str[i] == '\\' && i + 1 < str.size())
            {
                char esc = str[i + 1];
                switch (esc)
                {
                    case 'n':  out += '\n'; ++i; continue;
                    case 'r':  out += '\r'; ++i; continue;
                    case 'b':  out += '\b'; ++i; continue;
                    case 't':               ++i; continue;
                    case '"':  out += '"';  ++i; continue;
                    case '\\': out += '\\'; ++i; continue;
                    default: break;
                }
            }
            else if (str[i] == '{')
            {
                std::size_t close = str.find('}', i);
                if (close != std::string::npos)
                {
                    std::string_view tag(str.data() + i + 1, close - i - 1);
                    if (!tag.empty() && tag.find(' ') == std::string::npos)
                    {
                        auto it = map.find(tag);
                        if (it != map.end()) { out += it->second; i = close; continue; }
                    }
                    out += '{';
                }
                else out += str[i];
                continue;
            }
            out += str[i];
        }
        return out;
    }

    void KBeginImpl(const std::string& cmdtitle, const std::string& title,
                    const std::string& description, const std::string& author,
                    const std::string& date)
    {
        
        
        GLOBAL = ReadKsonFile("config/global.kson")["global"]["data"];

        

        auto splitLines = [](const std::string& s) {
            std::vector<std::string> lines;
            std::string cur;
            for (char c : s)
            {
                if (c == '\n') { lines.push_back(cur); cur.clear(); }
                else cur.push_back(c);
            }
            lines.push_back(cur);
            return lines;
        };

        struct LineInfo { std::string content; bool highlight; };
        std::vector<LineInfo> lines;

        for (auto& l : splitLines(title))       lines.push_back({Kout::ParseTags(l), false});
        lines.push_back({"", false});
        for (auto& l : splitLines(description)) lines.push_back({Kout::ParseTags(l), false});
        lines.push_back({"", false});
        {
            std::string authorLabel = "author: ";
            std::string authorLine = authorLabel + (author.empty() ? KBEGIN_UNKNOWN : author);
            lines.push_back({Kout::ParseTags(authorLine), true});
        }
        {
            std::string dateLabel = "date: ";
            std::string dateLine = dateLabel + (date.empty() ? KBEGIN_UNKNOWN : date);
            lines.push_back({Kout::ParseTags(dateLine), true});
        }

        size_t w = 0;
        for (auto& li : lines) { size_t dw = DisplayWidthNoAnsi(li.content); if (dw > w) w = dw; }

        std::string bar(w + 4, '-');
        std::cout << SkyBlue << "+" << bar << "+\n";
        for (auto& li : lines)
        {
            size_t pad = w - DisplayWidthNoAnsi(li.content);
            std::cout << SkyBlue << "|  ";
            if (li.highlight)
                std::cout << LightYellow << li.content << std::string(pad, ' ');
            else
                std::cout << li.content << std::string(pad, ' ');
            std::cout << SkyBlue << "  |\n";
        }
        std::cout << "+" << bar << "+" << Reset << "\n";
        std::cout << '\n';
    }

    void KBegin(const std::vector<std::string>& args)
    {
        // 元组取 cmdtitle（首参即窗口标题兼标题）
        std::string cmdtitle = args.empty() ? "" : args[0];
        SetConsoleOutputCP(CP_UTF8);
        SetTitleUTF8(cmdtitle);
        EnableVT100();
        // 语义: {cmdtitle, title, description, author, date}
        auto get = [&args](size_t i, const char* def = KBEGIN_UNKNOWN) {
            return (i < args.size() && !args[i].empty()) ? args[i] : std::string(def);
        };
        std::string title = get(0, "");
        std::string description = get(1, "");
        std::string author = get(2);
        std::string date   = get(3);
        KBeginImpl(cmdtitle, title, description, author, date);
    }

    std::size_t KOptions(const std::vector<std::string>& options, const std::string& title)
    {
        std::size_t count = options.size();
        if (count == 0)
        {
            std::cout << Red << "错误：菜单没有可选项" << Reset << "\n";
            return 0;
        }

        size_t maxW = DisplayWidth(title);
        for (std::size_t i = 0; i < count; i++)
        {
            std::string prefix = "  [" + std::to_string(i + 1) + "] ";
            size_t lineW = DisplayWidth(prefix) + DisplayWidth(options[i]);
            if (lineW > maxW) maxW = lineW;
        }
        size_t innerW = maxW + 2;

        std::string bar(innerW, '-');
        std::cout << "\n" << SkyBlue << "+" << bar << "+\n";

        if (!title.empty())
        {
            size_t titleW = DisplayWidth(title);
            size_t leftPad = (innerW - titleW) / 2;
            size_t rightPad = innerW - titleW - leftPad;
            std::cout << "|" << std::string(leftPad, ' ')
                      << Bold << title << Reset << SkyBlue
                      << std::string(rightPad, ' ') << "|\n";
            std::cout << "+" << bar << "+\n";
        }

        for (std::size_t i = 0; i < count; i++)
        {
            std::string prefix = "  [" + std::to_string(i + 1) + "] ";
            std::string content = prefix + options[i];
            size_t pad = innerW - DisplayWidth(content);
            std::cout << "|" << content << std::string(pad, ' ') << "|\n";
        }

        std::cout << "+" << bar << "+" << Reset << "\n";

        while (true)
        {
            std::cout << SkyBlue << "请输入选择 [1-" << count << "]: " << Reset;
            std::string line;
            std::getline(std::cin, line);
            try {
                size_t choice = std::stoull(line);
                if (choice >= 1 && choice <= count)
                    return choice - 1;
            } catch (...) {}
            std::cout << Red << "输入超出范围，请重新输入" << Reset << "\n";
        }
    }

    void KPause()
    {
        std::cout << "\n" << SkyBlue << "按任意键继续..." << Reset << std::flush;
        system("pause >nul");
        std::cout << "\n";
    }

    void KEnd()
    {
        RestoreConsoleFont();
        KPause();
        exit(0);
    }
