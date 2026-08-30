#pragma once
#include "KFCommon.hpp"
#include "KLOGGER.hpp"
#include "KMATH.hpp"
#include "KSON.hpp"
namespace KF
{
    namespace KCLI
    {
        // Color 常量统一定义在 KF::KLOGGER::Color，此处创建别名以便 KCLI 内直接使用 Color::xxx
        namespace Color = KF::KLOGGER::Color;

        /// @brief 链式输出流，自带默认颜色
        /// @details 每次 << 自动套用默认色；遇到 std::endl 等 manipulator 时
        ///          先输出 Color::Reset 再输出 manipulator，防止颜色泄漏。
        ///          支持在字符串中使用 {tag} 格式的颜色标签：
        ///          {red} {green} {blue} {yellow} {skyblue} {orange} {magenta} {cyan} {lightyellow}
        ///          {bold} {dim} {underline} {blink}
        ///          {bg_red} {bg_blue} ...
        ///          {/} 重置所有, {/bold} 重置单个
        /// @code
        /// kout  << "{red}红字{/} 普通信息" << std::endl;
        /// koutW << "{bold}{yellow}警告{/} 信息" << std::endl;
        /// @endcode
        class Kout
        {
            const char* color_;
        public:
            explicit constexpr Kout(const char* c) noexcept : color_(c) {}

            // 核心 tag 解析：扫描字符串，替换 {tag} 为 ANSI 码
            static std::string ParseTags(std::string_view str);

            template<typename T>
            Kout& operator<<(const T& val)
            {
                std::cout << color_ << val;
                return *this;
            }

            // 针对 std::string 特化，自动解析 {tag}
            Kout& operator<<(const std::string& val)
            {
                std::cout << color_ << ParseTags(val);
                return *this;
            }

            // 针对 const char* 特化
            Kout& operator<<(const char* val)
            {
                std::cout << color_ << ParseTags(std::string_view(val));
                return *this;
            }

            /// manipulator（std::endl / std::flush）：先 Reset 再输出
            Kout& operator<<(std::ostream& (*manip)(std::ostream&))
            {
                std::cout << Color::Reset << manip;
                return *this;
            }
        };

        /// @brief 链式输入会话：\c kin >> a >> b >> c 读入一组（空格/换行分隔）值，
        ///        自动按变量类型转换；任一非法则丢弃整组、给出醒目提示并整组重新输入，直到全部合法。
        /// @note  以分号结尾的每条 \c kin >> ... 语句为一组，值的个数须与变量个数一致。
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

            /// 收集一个变量及其按类型生成的解析器
            template<typename T>
            KinSession&& operator>>(T& val)
            {
                setters_.push_back(makeSetter(val));
                return std::move(*this);
            }

        private:
            /// 为不同类型的变量生成「token -> bool(是否合法)」的解析器
            template<typename T>
            static std::function<bool(const std::string&)> makeSetter(T& val)
            {
                if constexpr (std::is_same_v<T, ::KF::KMATH::BigDec>)
                {
                    return [&val](const std::string& t) -> bool {
                        try { val = ::KF::KMATH::BigDec(t); return true; }
                        catch (...) { return false; }
                    };
                }
                else if constexpr (std::is_same_v<T, std::size_t>) // 支持完整 size_t 范围（>LLONG_MAX 也可读入）
                {
                    return [&val](const std::string& t) -> bool {
                        try {
                            if (!t.empty() && t[0] == '-') return false; // 负数对无符号 size_t 非法 → 整组重输
                            std::size_t idx = 0;
                            const unsigned long long v = std::stoull(t, &idx);
                            if (idx != t.size() || v > (std::numeric_limits<std::size_t>::max)()) return false;
                            val = static_cast<std::size_t>(v);
                            return true;
                        }
                        catch (...) { return false; }
                    };
                }
                else if constexpr (std::is_same_v<T, std::string>)// 如果是字符串
                {
                    return [&val](const std::string& t) -> bool { val = t; return true; };
                }
                else if constexpr (std::is_same_v<T, bool>) // 如果是布尔值
                {
                    // 大小写不敏感；支持常见真/假写法
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
                    // 严格整数：必须整个 token 都是整数，带小数点等视为非法
                    // 对于无符号类型 (size_t, uint, unsigned int 等)，只接受正整数，否则返回 false 走整组重试
                    return [&val](const std::string& t) -> bool {
                        try {
                            std::size_t idx = 0;
                            const long long v = std::stoll(t, &idx);
                            if (idx != t.size()) return false; // 如 "3.14" 解析到 '.' 即停，判非法
                            if constexpr (std::is_unsigned_v<T>) {
                                if (v < 0) return false; // 非正整数，走整组重试
                            }
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

            /// 整组读取：读够 N 个 token，全部合法才提交；否则醒目提示并整组重试
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
                        if (!(std::cin >> t)) return; // EOF：放弃本次输入
                        tokens.push_back(t);
                    }
                    bool allok = true;
                    for (std::size_t i = 0; i < n; ++i)
                        if (!setters_[i](tokens[i])) { allok = false; break; }
                    if (allok) return;
                    // 醒目提示，整组重新输入
                    Kout(Color::Orange) << "\n输入不合法，请重新输入整组（以空格分隔，共 " << n << " 个值）：" << std::endl;
                }
            }
        };

        /// @brief 链式输入流，\c kin >> a >> b >> c 自动按变量类型读取并整组校验
        /// @code int x; bool b; kin >> x >> b; @endcode
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

        inline Kout kout(Color::Bold);      ///< 默认天蓝色（普通输出）
        inline Kout koutW(Color::LightYellow); ///< 默认淡黄色（Warning）
        inline Kout koutE(Color::Orange);  ///< 默认橙色（Error）
        inline Kout koutF(Color::Red);     ///< 默认红色（Fatal）
        inline Kin kin;                    ///< 全局输入流对象（链式 >> ，自动推导类型）

        /////////////////////////////////////////////////////////
        // CLI 功能函数
        /////////////////////////////////////////////////////////

        /// @brief 初始化 CLI：启用 VT100 颜色，设置控制台标题（支持中文），
        ///        打印标题框和描述
        /// 字符串版本（可变参数，定义见下方模板）：
        ///   KBegin(title, desc)                  // 2 参
        ///   KBegin(title, desc, author)          // 3 参
        ///   KBegin(title, desc, author, date)    // 4 参
        /// @param title 标题（留空则不打印标题框）
        /// @param description 描述（留空则不打印描述）
        /// @param author 作者（亮黄显示）
        /// @param date 日期（亮黄显示）
        void KBeginImpl(const std::string& cmdtitle, const std::string& title,
                        const std::string& description, const std::string& author,
                        const std::string& date);

        template<typename... Args>
        void KBegin(const std::string& a, const std::string& b, Args... rest)
        {
            std::vector<std::string> args{a, b, rest...};
            // 语义: [title, desc] / [title, desc, author] / [title, desc, author, date]
            std::string title = a;
            std::string description = b;
            std::string author = args.size() >= 3 ? args[2] : "";
            std::string date   = args.size() >= 4 ? args[3] : "";
            KBeginImpl(title, title, description, author, date);
        }

        void KBegin(const KSON::kson file);//从文件中读取

        /// @brief 全局配置：KBegin 时自动读取 config/global.kson 的 data 节点
        /// @details 程序内任意位置可直接使用 GLOBAL["键"] 读取全局配置项
        extern KSON::kson GLOBAL;
        /// @brief 显示选项菜单，循环等待用户输入合法选项
        /// @param menu KSON 节点，需含 "title" 和 "options"（字符串数组）
        /// @return 选中项索引（0-based），输入非法时循环提示
        std::size_t KOptions(const KSON::kson& menu);

        /// @brief 暂停：显示"按任意键继续..."并等待按键
        void KPause();

        /// @brief 结束：暂停后退出程序（exit(0)）
        void KEnd();
        ////////////////////////////////////////杂函数/////////////////////////

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
        /// @brief 经典 cmd 下自动分级适配（先调窗口→缩字号到 6→全屏）；keepIfFits=true 时内容过小则保持原窗口
        /// @param rows 需要显示的行数
        /// @param cols 需要显示的列数
        /// @param keepIfFits 内容过小时保持原窗口（迷宫场景传 true）
        /// @return 恒为 true（cmd 下总是尽力适配）
        bool CheckConsoleFit(int rows, int cols, bool keepIfFits = false);

        /// @brief ANSI 清屏（光标归位+清除屏幕），替代 system("cls") 避免闪屏
        void ClearScreen();
    }
}
