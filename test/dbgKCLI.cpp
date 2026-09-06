/**
/**
 * @file    dbgKCLI.cpp
 * @brief   KCLI 命令行交互模块全功能测试
 *
 * 测试内容:
 *   1. kout 链式输出 (天蓝色, 多类型)
 *   2. koutW 链式输出 (淡黄色)
 *   3. koutE 链式输出 (橙色)
 *   4. koutF 链式输出 (红色)
 *   5. 临时换色
 *   6. Color 常量展示
 *   7. kin 链式输入 (int / double / string / bool)
 *   8. KOptions 菜单 (字符串配置)
 *   9. 从文件读取配置的 KOptions
 *  10. kpause / KEnd
 */
import kf;
#include <iostream>
#include <string>

// ==================== 测试辅助宏 ====================
#define SECTION(name) kout << Bold << "\n--- " << name << " ---" << Reset << std::endl

int main()
{
    kson doc = ReadKsonFile("config/test/cfg.kson");
    kson main = doc["dbgKCLI"];
    KBegin(main["meta"].Vec());
    // ==================== 1. kout 链式输出 ====================
    SECTION("1. kout 链式输出");
    {
        kout << "  整数: " << 42 << std::endl;
        kout << "  浮点: " << 3.14 << std::endl;
        kout << "  字符串: " << "Hello" << std::endl;
        kout << "  字符: " << 'A' << std::endl;
        kout << "  布尔(1): " << true << std::endl;
        kout << "  混合: int=" << 10 << ", dec=" << 2.5 << ", str=" << "test" << std::endl;
    }

    // ==================== 2. koutW 链式输出 ====================
    SECTION("2. koutW 链式输出 (淡黄色)");
    {
        koutW << "  警告: 配置项缺失" << std::endl;
        koutW << "  警告: code=" << 100 << ", msg=timeout" << std::endl;
    }

    // ==================== 3. koutE 链式输出 ====================
    SECTION("3. koutE 链式输出 (橙色)");
    {
        koutE << "  错误: 文件未找到" << std::endl;
        koutE << "  错误: code=" << 404 << ", path=/cfg.kson" << std::endl;
    }

    // ==================== 4. koutF 链式输出 ====================
    SECTION("4. koutF 链式输出 (红色)");
    {
        koutF << "  致命: 内存耗尽" << std::endl;
        koutF << "  致命: code=" << 500 << ", 无法恢复" << std::endl;
    }

    // ==================== 5. 临时换色 ====================
    SECTION("5. 临时换色");
    {
        kout << "  默认天蓝色输出" << std::endl;
        kout << Red << "  临时红色" << Reset << std::endl;
        kout << "  恢复天蓝色" << std::endl;
        kout << Green << "  临时绿色" << Reset << std::endl;
        kout << Bold << "  加粗文本" << Reset << std::endl;
        kout << Magenta << "  临时紫红" << Reset << std::endl;
    }

    // ==================== 6. Color 常量展示 ====================
    SECTION("6. Color 常量展示");
    {
        kout << Red         << "  Red"          << Reset << std::endl;
        kout << Green       << "  Green"        << Reset << std::endl;
        kout << Yellow      << "  Yellow"       << Reset << std::endl;
        kout << Blue        << "  Blue"         << Reset << std::endl;
        kout << Magenta     << "  Magenta"      << Reset << std::endl;
        kout << Cyan        << "  Cyan"         << Reset << std::endl;
        kout << LightYellow << "  LightYellow"  << Reset << std::endl;
        kout << Orange      << "  Orange"       << Reset << std::endl;
        kout << SkyBlue     << "  SkyBlue (kout 默认)" << Reset << std::endl;
        kout << Bold        << "  Bold"         << Reset << std::endl << std::endl;
        kout << doc["dbgKSON"]["escapes"]["color"].Str() << std::endl;
    }

    // ==================== 7. kin 链式输入 ====================
    SECTION("7. kin 链式输入 (自动类型推导)");
    {
        kout << "  请依次输入 (每项一行):" << std::endl;
        kout << "  1. 整数: ";
        int i;
        kin >> i;

        kout << "  2. 浮点数: ";
        double d;
        kin >> d;

        kout << "  3. 字符串: ";
        std::string s;
        kin >> s;

        kout << "  4. 布尔值 (1/0): ";
        bool b;
        kin >> b;

        kout << Bold << "  --- 输入结果 ---" << Reset << std::endl;
        kout << "  int=" << i << ", dec=" << d << ", str=" << s << ", bool=" << b << std::endl;
    }

    // ==================== 7.1 kin 无符号类型支持 ====================
    SECTION("7.1 kin 无符号类型 (size_t, uint 等)");
    {
        kout << "  请输入一个正整数 (unsigned int): ";
        unsigned int ui;
        kin >> ui;
        kout << "  你输入的 unsigned int = " << ui << std::endl;

        kout << "  请输入一个正整数 (size_t): ";
        size_t st;
        kin >> st;
        kout << "  你输入的 size_t = " << st << std::endl;

        koutW << "  测试负数输入 (会触发警告): ";
        // 注意：若输入 -5，Kin 会警告并整组重试
        // 这里仅验证类型支持，实际警告由 KinSession 在提交时处理
    }

    // ==================== 8. KOptions 菜单 (字符串配置) ====================
    SECTION("8. KOptions 菜单 (字符串配置)");
    {

        kson menu = read(Preprocess(
            "\"title\": \"请选择操作\","
            "\"options\": [\"选项 A\", \"选项 B\", \"选项 C\", \"返回\"]"
        ));
        size_t choice = KOptions(menu["options"].Vec(), menu["title"].Auto());
        kout << "  你选择了: [" << choice << "] " << menu["options"][choice].Auto() << std::endl;
    }

    // ==================== 9. 从文件读取配置 ====================
    SECTION("9. 从文件读取配置的 KOptions");
    {
        kson menu = main["KOption"];
        size_t choice = KOptions(menu["options"].Vec(), menu["title"].Auto());
        kout << "  你选择了: [" << choice << "] " << menu["options"][choice].Auto() << std::endl;
    }

    // ==================== 10. kpause ====================
    SECTION("10. kpause 暂停");
    {
        kout << "  测试 kpause() 暂停功能" << std::endl;
        KPause();
        kout << "  暂停结束, 继续执行" << std::endl;
    }

    // ==================== 11. 从 KSON 读取 inf / -inf / nan ====================
    SECTION("11. 从 KSON 读取 inf / -inf / nan");
    {
        kson in = main["inf_nan"];
        if(!in.Exists())
        {
            koutE << "  缺少 inf_nan 键, 跳过" << std::endl;
        }
        else
        {
            // 关键字形式（存储为 double 特殊值）
            double inf     = in["inf"].Dec();
            double neg_inf = in["neg_inf"].Dec();
            double nan     = in["nan"].Dec();
            kout << "  inf     关键字: " << inf     << "  (IsInf=" << std::isinf(inf)     << ")" << std::endl;
            kout << "  -inf    关键字: " << neg_inf << "  (IsInf=" << std::isinf(neg_inf) << ")" << std::endl;
            kout << "  nan     关键字: " << nan     << "  (IsNan=" << std::isnan(nan)     << ")" << std::endl;

            // 字符串形式（大小写不敏感）
            double str_inf = in["str_inf"].Dec();
            double str_nan = in["str_nan"].Dec();
            kout << "  \"inf\"  字符串: " << str_inf << "  (IsInf=" << std::isinf(str_inf) << ")" << std::endl;
            kout << "  \"NaN\"  字符串: " << str_nan << "  (IsNan=" << std::isnan(str_nan) << ")" << std::endl;

            // 数组中的 inf / -inf / nan
            kson arr = in["array"];
            for(size_t i = 0; i < arr.Size(); i++)
                kout << "  array[" << i << "] = " << arr[i].Dec() << std::endl;
        }
    }

    // ==================== 12. 迷宫读取 + 打印 ====================
    SECTION("12. 迷宫读取 + 打印");
    {
        kout << "  Reading maze from config/test/maze.kson..." << std::endl;
        auto maze = ReadMaze("config/test/maze.kson", "small");
        kout << "  Maze size: " << maze.size() << " rows x "
             << (maze.empty() ? 0 : maze[0].size()) << " cols" << std::endl;
        kout << "  Rendering (red=wall, green=start, blue=end):" << std::endl;
        Maze::Print(maze);
    }

    // ==================== 完成 ====================
    kout << Bold << "\n=== dbgKCLI 所有测试完成 ===" << Reset << std::endl;

    // KEnd: 暂停后退出
    KEnd();
}
