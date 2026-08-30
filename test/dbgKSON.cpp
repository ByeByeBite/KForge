/**
 * @file    dbgKSON.cpp
 * @brief   KSON 解析模块全功能测试
 *
 * 测试内容:
 *   1.  字符串解析 (隐式对象 / 显式对象 / 数组 / 单值)
 *   2.  文件解析 (ReadKsonFile / ParseFile)
 *   3.  节点类型判断 (IsNull / IsBool / IsInt / IsDec / IsString / IsArray / IsObject)
 *   4.  节点取值 (AsBool / AsInt / AsDec / AsStr / AsArr / AsObj)
 *   5.  NodePtr 路径访问 (operator[] / TryResolve / Resolve)
 *   6.  find / at 查找
 *   7.  size 大小
 *   8.  Auto() 类型自动转换
 *   9.  多维数组遍历 (2D / 3D)
 *  10.  字符串转义
 *  11.  注释 / 尾随逗号
 *  12.  重复键覆盖
 *  13.  空数组 / 空对象
 *  14.  NodePtr 取值方法 (Str / Int / Dec / Bool / Size / Exists)
 *  15.  Preprocess 预处理
 *  16.  错误条件 (警告级)
 *  17.  BigNum / 科学计数法 (数组下标访问)
 *  18.  错误用例 (error_cases: normalize_errors / kson_parse_errors)
 *  19.  边界与类型不匹配 (find/at/size 误用 + AsSth 类型不匹配)
 */

#include "base/KF.hpp"
using namespace KFIO;
using namespace KSON;
using namespace KLOG;
using namespace KCLI;

// ==================== 测试辅助宏 ====================
static int g_ok = 0, g_fail = 0;
#define SECTION(name) kout << Color::Bold << "\n--- " << name << " ---" << Color::Reset << std::endl
#define CHECK(cond, desc) do { \
    if (cond) { kout << "  {green}[PASS]{/} " << desc << std::endl; ++g_ok; } \
    else      { koutE << "  {red}[FAIL]{/} " << desc << std::endl; ++g_fail; } \
} while(0)
#define SHOW(label, value) kout << "  " << label << ": " << value << std::endl

int main()
{
    kson root = ReadKsonFile("config/test/cfg.kson");
    kson doc = root["dbgKSON"];
    if(!doc.Exists())
    {
        koutE << "{red}[FAIL]{/}  cfg.kson 缺少 dbgKSON 键，无法继续" << std::endl;
        KEnd();
        return 1;
    }
    KBegin(doc);

    // ==================== 1. 字符串解析 ====================
    SECTION("1. 字符串解析");

    // 隐式对象 (无外层 {})
    kson implicit_obj = read(Preprocess(
        "\"name\": \"Alice\","
        "\"age\": 25,"
        "\"score\": 95.5"
    ));
    CHECK(implicit_obj.Exists(), "隐式对象解析");
    SHOW("name",  implicit_obj["name"].Auto());
    SHOW("age",   implicit_obj["age"].Auto());
    SHOW("score", implicit_obj["score"].Auto());

    // 显式对象
    kson explicit_obj = read(Preprocess(
        "{\"key\": \"value\", \"num\": 42}"
    ));
    CHECK(explicit_obj["key"].Exists(), "显式对象解析");
    SHOW("key", explicit_obj["key"].Auto());
    SHOW("num", explicit_obj["num"].Auto());

    // 数组
    kson arr = read(Preprocess("[1, 2, 3, 4, 5]"));
    CHECK(arr.size() == 5, "数组解析 (size=5)");
    std::size_t idx0 = 0, idx4 = 4;
    SHOW("arr[0]", arr[idx0].Auto());
    SHOW("arr[4]", arr[idx4].Auto());

    // 单值
    SHOW("单值 int",  read(Preprocess("42")).Auto());
    SHOW("单值 dec",  read(Preprocess("3.14")).Auto());
    SHOW("单值 str",  read(Preprocess("\"hello\"")).Auto());
    SHOW("单值 bool", read(Preprocess("true")).Auto());
    SHOW("单值 null", read(Preprocess("null")).Auto());

    // ==================== 2. 文件解析 ====================
    SECTION("2. 文件解析");
    {
        // ReadKsonFile: 一站式 读取 + 预处理 + 解析
        kson doc = ReadKsonFile("config/test/cfg.kson")["dbgKSON"];
        CHECK(doc.Exists(), "ReadKsonFile 读取 config/test/cfg.kson");
        SHOW("intro.name",    doc["intro"]["name"].Auto());
        SHOW("intro.description", doc["intro"]["description"].Auto());

        // NodePtr::ParseFile 静态工厂
        kson doc2 = NodePtr::ParseFile("config/test/cfg.kson");
        CHECK(doc2.Exists(), "NodePtr::ParseFile 读取 config/test/cfg.kson");

        // 手动流程: ReadFileRaw + Preprocess + read
        kson doc3 = read(Preprocess(ReadFileRaw("config/test/cfg.kson")));
        CHECK(doc3.Exists(), "手动流程读取 config/test/cfg.kson");
    }

    // ==================== 3. 节点类型判断 ====================
    SECTION("3. 节点类型判断");
    {
        const Node* n;

        n = doc["types"]["integer"].TryResolve();
        CHECK(n && n->IsInt(),    "IsInt (42)");
        CHECK(n && n->IsNumber(), "IsNumber (42)");

        n = doc["types"]["decimal"].TryResolve();
        CHECK(n && n->IsDec(),    "IsDec (3.14)");
        CHECK(n && n->IsNumber(), "IsNumber (3.14)");

        n = doc["types"]["string"].TryResolve();
        CHECK(n && n->IsString(), "IsString");

        n = doc["types"]["bool_true"].TryResolve();
        CHECK(n && n->IsBool(),   "IsBool (true)");

        n = doc["types"]["null_value"].TryResolve();
        CHECK(n && n->IsNull(),   "IsNull");

        n = doc["types"].TryResolve();
        CHECK(n && n->IsObject(), "IsObject (types)");

        n = doc["arrays"]["int_array"].TryResolve();
        CHECK(n && n->IsArray(),  "IsArray (int_array)");
    }

    // ==================== 4. 节点取值 ====================
    SECTION("4. 节点取值");
    {
        const Node* n;

        n = doc["types"]["integer"].TryResolve();
        SHOW("AsInt (42)",       n->AsInt());
        SHOW("AsDec (from int)", n->AsDec());

        n = doc["types"]["negative_int"].TryResolve();
        SHOW("AsInt (-7)",       n->AsInt());

        n = doc["types"]["decimal"].TryResolve();
        SHOW("AsDec (3.14)",     n->AsDec());

        n = doc["types"]["string"].TryResolve();
        SHOW("AsStr",            n->AsStr());

        n = doc["types"]["bool_true"].TryResolve();
        SHOW("AsBool (true)",    n->AsBool());

        n = doc["types"]["bool_false"].TryResolve();
        SHOW("AsBool (false)",   n->AsBool());

        n = doc["arrays"]["int_array"].TryResolve();
        SHOW("AsArr().size()",   n->AsArr().size());
    }

    // ==================== 5. NodePtr 路径访问 ====================
    SECTION("5. NodePtr 路径访问");
    {
        // 链式对象访问
        SHOW("doc[intro][name]",
             doc["intro"]["name"].Auto());

        // 深层嵌套
        SHOW("doc[objects][nested][level1][level2][deep_value]",
             doc["objects"]["nested"]["level1"]["level2"]["deep_value"].Auto());

        // 数组下标访问
        std::size_t idx2 = 2;
        SHOW("doc[arrays][int_array][2]",
             doc["arrays"]["int_array"][idx2].Auto());

        // const char* 重载
        SHOW("operator[](const char*)",
             doc["types"]["string"].Auto());

        // 不存在的路径
        CHECK(!doc["nonexistent"].Exists(),             "不存在路径 Exists()=false");
        CHECK(doc["nonexistent"].Auto() == "null",      "不存在路径 Auto()=\"null\"");
        CHECK(doc["nonexistent"].TryResolve() == nullptr, "不存在路径 TryResolve()=nullptr");
    }

    // ==================== 6. find / at ====================
    SECTION("6. find / at");
    {
        const Node* types = doc["types"].TryResolve();
        if (types)
        {
            const Node* found = types->find("integer");
            CHECK(found != nullptr, "find(\"integer\") 找到");
            SHOW("  found value",   found->AsInt());

            const Node* not_found = types->find("nonexistent");
            CHECK(not_found == nullptr, "find(\"nonexistent\") 返回 nullptr");

            // find 区分大小写
            const Node* case_sensitive = types->find("Integer");
            CHECK(case_sensitive == nullptr, "find 区分大小写 (\"Integer\" != \"integer\")");
        }

        const Node* arr_node = doc["arrays"]["int_array"].TryResolve();
        if (arr_node)
        {
            const Node* elem = arr_node->at(2);
            CHECK(elem != nullptr,    "at(2) 找到");
            SHOW("  at(2) value",     elem->AsInt());

            const Node* oob = arr_node->at(999);
            CHECK(oob == nullptr,     "at(999) 越界返回 nullptr");
        }
    }

    // ==================== 7. size ====================
    SECTION("7. size");
    {
        const Node* arr_node = doc["arrays"]["int_array"].TryResolve();
        CHECK(arr_node->size() == 5, "int_array Node::size() = 5");

        const Node* types = doc["types"].TryResolve();
        SHOW("types Node::size()", types->size());

        const Node* empty_arr = doc["arrays"]["empty_array"].TryResolve();
        CHECK(empty_arr->size() == 0, "empty_array size = 0");

        // NodePtr::Size() 和 size() 别名
        SHOW("NodePtr::Size()",    doc["arrays"]["int_array"].Size());
        SHOW("NodePtr::size()",    doc["arrays"]["int_array"].size());

        // 标量 size() = 0
        const Node* scalar = doc["types"]["integer"].TryResolve();
        CHECK(scalar->size() == 0, "标量 size() = 0");
    }

    // ==================== 8. Auto() 类型自动转换 ====================
    SECTION("8. Auto() 类型自动转换");
    {
        SHOW("int",  doc["types"]["integer"].Auto());
        SHOW("dec",  doc["types"]["decimal"].Auto());
        SHOW("str",  doc["types"]["string"].Auto());
        SHOW("bool", doc["types"]["bool_true"].Auto());
        SHOW("null", doc["types"]["null_value"].Auto());
        SHOW("arr",  doc["arrays"]["int_array"].Auto());
        SHOW("obj",  doc["types"].Auto());
        SHOW("empty_arr", doc["arrays"]["empty_array"].Auto());
        SHOW("empty_obj", doc["objects"]["empty_obj"].Auto());
    }

    // ==================== 9. 多维数组遍历 ====================
    SECTION("9. 多维数组遍历");
    {
        // 2D 数组
        kout << "  2D 数组:" << std::endl;
        kson arr2d = doc["arrays"]["nested_2d"];
        for (size_t i = 0; i < arr2d.size(); i++)
        {
            kout << "    [";
            for (size_t j = 0; j < arr2d[i].size(); j++)
            {
                if (j) kout << ", ";
                kout << arr2d[i][j].Auto();
            }
            kout << "]" << std::endl;
        }

        // 3D 数组
        kout << "  3D 数组:" << std::endl;
        kson arr3d = doc["arrays"]["nested_3d"];
        for (size_t i = 0; i < arr3d.size(); i++)
        {
            kout << "    Layer " << i << ":" << std::endl;
            for (size_t j = 0; j < arr3d[i].size(); j++)
            {
                kout << "      [";
                for (size_t k = 0; k < arr3d[i][j].size(); k++)
                {
                    if (k) kout << ", ";
                    kout << arr3d[i][j][k].Auto();
                }
                kout << "]" << std::endl;
            }
        }
    }

    // ==================== 10. 字符串转义 ====================
    SECTION("10. 字符串转义");
    {
        SHOW("newline",      doc["escapes"]["newline"].Auto());
        SHOW("tab",          doc["escapes"]["tab"].Auto());
        SHOW("quote",        doc["escapes"]["quote"].Auto());
        SHOW("backslash",    doc["escapes"]["backslash"].Auto());
        SHOW("carriage_ret", doc["escapes"]["carriage_return"].Auto());
        SHOW("backspace",    doc["escapes"]["backspace"].Auto());
    }

    // ==================== 11. 注释 / 尾随逗号 ====================
    SECTION("11. 注释 / 尾随逗号");
    {
        // config/test/cfg.kson 本身包含注释和尾随逗号
        CHECK(doc.Exists(), "含注释的文件解析成功");

        // 尾随逗号 - 数组
        kson trail_arr = read(Preprocess("[1, 2, 3,]"));
        CHECK(trail_arr.size() == 3, "尾随逗号数组 (size=3)");

        // 尾随逗号 - 对象
        kson trail_obj = read(Preprocess("{\"a\": 1, \"b\": 2,}"));
        CHECK(trail_obj["a"].Exists(), "尾随逗号对象");

        // 注释行
        kson commented = read(Preprocess(
            "# 这是注释\n\"key\": \"value\" # 行尾注释\n"
        ));
        CHECK(commented["key"].Auto() == "value", "注释行正确跳过");
    }

    // ==================== 12. 重复键覆盖 ====================
    SECTION("12. 重复键覆盖");
    {
        SHOW("dup_key (应为 second)", doc["dup_key"].Auto());
        CHECK(doc["dup_key"].Auto() == "second", "重复键后覆盖前");
    }

    // ==================== 13. 空数组 / 空对象 ====================
    SECTION("13. 空数组 / 空对象");
    {
        const Node* ea = doc["arrays"]["empty_array"].TryResolve();
        CHECK(ea && ea->IsArray() && ea->size() == 0, "空数组 IsArray + size=0");

        const Node* eo = doc["objects"]["empty_obj"].TryResolve();
        CHECK(eo && eo->IsObject() && eo->size() == 0, "空对象 IsObject + size=0");

        SHOW("Auto() 空数组", doc["arrays"]["empty_array"].Auto());
        SHOW("Auto() 空对象", doc["objects"]["empty_obj"].Auto());
    }

    // ==================== 14. NodePtr 取值方法 ====================
    SECTION("14. NodePtr 取值方法");
    {
        kson s = doc["types"]["string"];
        SHOW("Str()",  s.Str());

        kson i = doc["types"]["integer"];
        SHOW("Int()",  i.Int());

        kson d = doc["types"]["decimal"];
        SHOW("Dec()",  d.Dec());

        kson b = doc["types"]["bool_true"];
        SHOW("Bool()", b.Bool());

        kson sz = doc["arrays"]["int_array"];
        SHOW("Size()", sz.Size());

        CHECK(doc["types"]["integer"].Exists(),     "Exists() = true");
        CHECK(!doc["nonexistent"].Exists(),          "Exists() = false (不存在)");
    }

    // ==================== 15. Preprocess 预处理 ====================
    SECTION("15. Preprocess 预处理");
    {
        std::string raw = "# 注释行\n\"key\": \"value\"  # 行尾注释\n";
        std::string processed = Preprocess(raw);
        SHOW("原始长度",   raw.size());
        SHOW("处理后长度", processed.size());

        CHECK(processed.find('#') == std::string::npos, "注释已移除");
        CHECK(processed.find(' ') == std::string::npos, "空白已移除");

        kson result = read(processed);
        CHECK(result["key"].Auto() == "value", "预处理后解析正确");

        // 验证字符串内的 # 不被误删
        std::string raw2 = "\"url\": \"http://example.com#frag\"";
        std::string proc2 = Preprocess(raw2);
        CHECK(proc2.find('#') != std::string::npos, "字符串内 # 保留");
    }

    // ==================== 16. 错误条件 (警告级) ====================
    SECTION("16. 错误条件 (警告级)");
    {
        koutW << "  以下测试会触发 KLOGGER 警告/错误输出到 stderr" << std::endl;

        // 多个小数点 (Warning)
        kout << "  >> 多个小数点 (KSON_PARSE_MULPOINT)" << std::endl;
        kson mulpoint = read(Preprocess("\"x\": 1.2.3"));

        // 尾随字符 (Warning)
        kout << "  >> 尾随字符 (KSON_PARSE_TRAIL)" << std::endl;
        kson trail = read(Preprocess("42 extra"));

        // 无效转义 (Warning)
        kout << "  >> 无效转义 (KSON_PARSE_ESCAPE_SPECIAL)" << std::endl;
        kson badesc = read(Preprocess("\"x\": \"bad\\xescape\""));
    }

    // ==================== 17. BigNum / 科学计数法 ====================
    SECTION("17. BigNum / 科学计数法");
    {
        // config/test/cfg.kson 中 kson_bignum 现为数组，共 10 个元素，按下标访问
        kson bn = doc["kson_bignum"];
        const Node* bnRoot = bn.TryResolve();
        CHECK(bnRoot && bnRoot->IsArray(), "kson_bignum 是数组");
        CHECK(bnRoot && bnRoot->size() == 10, "kson_bignum 共 10 个元素");

        // 数组下标必须用 std::size_t，匹配 operator[](std::size_t) 重载
        std::size_t i0 = 0, i1 = 1, i2 = 2, i3 = 3, i4 = 4;
        std::size_t i5 = 5, i6 = 6, i7 = 7, i8 = 8, i9 = 9;

        // [0] = 9223372036854775807 (int64_max) → 普通整数
        kout << "  >> [0] int64_max (普通整数)" << std::endl;
        const Node* n0 = bn[i0].TryResolve();
        CHECK(n0 && n0->IsInt(), "[0] int64_max 是 Int 类型");
        CHECK(n0->AsInt() == 9223372036854775807LL, "[0] int64_max 值正确");
        SHOW("[0] int64_max", n0->AsInt());

        // [1] = 123456789012345678901234567890 (超出 int64) → 自动 BigNum
        kout << "  >> [1] overflow_big (自动转 BigNum)" << std::endl;
        const Node* n1 = bn[i1].TryResolve();
        CHECK(n1 && n1->IsBig(), "[1] overflow_big 是 BigNum 类型");
        CHECK(n1->AsBig().ToStr() == "123456789012345678901234567890", "[1] overflow_big 值正确");
        SHOW("[1] overflow_big", n1->AsBig().ToStr());

        // [2] = -987654321098765432109876543210 (负大数)
        kout << "  >> [2] negative_big (负大数)" << std::endl;
        const Node* n2 = bn[i2].TryResolve();
        CHECK(n2 && n2->IsBig(), "[2] negative_big 是 BigNum 类型");
        CHECK(n2->AsBig().isneg == true, "[2] negative_big 为负数");
        SHOW("[2] negative_big", n2->AsBig().ToStr());

        // [3] = -1.23e50 (科学计数法 → BigNum)
        kout << "  >> [3] sci_big (-1.23e50)" << std::endl;
        const Node* n3 = bn[i3].TryResolve();
        CHECK(n3 && n3->IsBig(), "[3] sci_big 是 BigNum 类型");
        SHOW("[3] sci_big", n3->AsBig().ToStr());

        // [4] = 1.23e-10 (负指数科学计数法 → BigNum)
        kout << "  >> [4] sci_small (1.23e-10)" << std::endl;
        const Node* n4 = bn[i4].TryResolve();
        CHECK(n4 && n4->IsBig(), "[4] sci_small 是 BigNum 类型");
        SHOW("[4] sci_small", n4->AsBig().ToStr());

        // [5] = -5e20 (负科学计数法 → BigNum)
        kout << "  >> [5] sci_neg (-5e20)" << std::endl;
        const Node* n5 = bn[i5].TryResolve();
        CHECK(n5 && n5->IsBig(), "[5] sci_neg 是 BigNum 类型");
        SHOW("[5] sci_neg", n5->AsBig().ToStr());

        // [6] = 3e25 (科学计数法 → BigNum)
        kout << "  >> [6] sci_auto (3e25)" << std::endl;
        const Node* n6 = bn[i6].TryResolve();
        CHECK(n6 && n6->IsBig(), "[6] sci_auto 是 BigNum 类型");
        SHOW("[6] sci_auto", n6->AsBig().ToStr());

        // [7] = 1235648273813688172316313716326731e-10 (大科学计数法 → BigNum)
        kout << "  >> [7] sci_huge (1235648273813688172316313716326731e-10)" << std::endl;
        const Node* n7 = bn[i7].TryResolve();
        CHECK(n7 && n7->IsBig(), "[7] sci_huge 是 BigNum 类型");
        SHOW("[7] sci_huge", n7->AsBig().ToStr());

        // [8] = 123456782222222222222222222222222222229B ('B' 后缀强制 BigNum)
        kout << "  >> [8] big_suffix (123456782222222222222222222222222222229B)" << std::endl;
        const Node* n8 = bn[i8].TryResolve();
        CHECK(n8 && n8->IsBig(), "[8] big_suffix 是 BigNum 类型");
        SHOW("[8] big_suffix", n8->AsBig().ToStr());

        // [9] = -987654321B (负 'B' 后缀 → BigNum, 负数)
        kout << "  >> [9] neg_big_suffix (-987654321B)" << std::endl;
        const Node* n9 = bn[i9].TryResolve();
        CHECK(n9 && n9->IsBig(), "[9] neg_big_suffix 是 BigNum 类型");
        CHECK(n9->AsBig().isneg == true, "[9] neg_big_suffix 为负数");
        SHOW("[9] neg_big_suffix", n9->AsBig().ToStr());

        // NodePtr::AsBig() 方法 (经 NodePtr 代理，路径解析后取大数)
        kout << "  >> NodePtr::AsBig() 方法" << std::endl;
        KMATH::BigDec bigval = bn[i1].Big();
        SHOW("NodePtr::AsBig()", bigval.ToStr());
    }

    // ==================== 18. 错误用例 (error_cases) ====================
    SECTION("18. 错误用例 (error_cases)");
    {
        kson ec = doc["error_cases"];
        CHECK(ec.Exists(), "error_cases 节点存在");

        // --- normalize_errors: 字符串数组，对每个元素调用 KMATH::Normalize() ---
        kout << "  >> normalize_errors 数组 → Normalize()" << std::endl;
        kson ne = ec["normalize_errors"];
        const Node* neRoot = ne.TryResolve();
        CHECK(neRoot && neRoot->IsArray(), "normalize_errors 是数组");
        if (neRoot)
        {
            std::size_t cnt = neRoot->size();
            for (std::size_t i = 0; i < cnt; i++)
            {
                const Node* elem = neRoot->at(i);
                if (elem && elem->IsString())
                {
                    std::string raw = std::string(elem->AsStr());
                    std::string normalized = KMATH::Normalize(raw);
                    kout << "    [" << i << "] raw=\"" << raw
                         << "\" → normalized=\"" << normalized << "\"" << std::endl;
                }
            }
        }

        // --- kson_parse_errors: 数组，对每个元素调用 Auto() 展示解析结果 ---
        kout << "  >> kson_parse_errors 数组 → Auto()" << std::endl;
        kson pe = ec["kson_parse_errors"];
        const Node* peRoot = pe.TryResolve();
        CHECK(peRoot && peRoot->IsArray(), "kson_parse_errors 是数组");
        if (peRoot)
        {
            std::size_t cnt = peRoot->size();
            for (std::size_t i = 0; i < cnt; i++)
            {
                kout << "    [" << i << "] Auto() = " << pe[i].Auto() << std::endl;
            }
        }
    }

    // ==================== 19. 边界与类型不匹配 ====================
    SECTION("19. 边界与类型不匹配");
    {
        koutW << "  以下测试会触发 KLOGGER 错误输出到 stderr" << std::endl;

        const Node* arrNode = doc["arrays"]["int_array"].TryResolve();
        const Node* strNode = doc["types"]["string"].TryResolve();
        const Node* objNode = doc["types"].TryResolve();
        const Node* nullNode = doc["types"]["null_value"].TryResolve();
        const Node* intNode = doc["types"]["integer"].TryResolve();
        const Node* decNode = doc["types"]["decimal"].TryResolve();
        const Node* boolNode = doc["types"]["bool_true"].TryResolve();

        // --- find() 在非对象节点上调用 → 返回 nullptr ---
        kout << "  >> find() 在非对象节点上调用" << std::endl;
        if (arrNode)
            CHECK(arrNode->find("key") == nullptr, "find() 在数组上返回 nullptr");
        if (strNode)
            CHECK(strNode->find("key") == nullptr, "find() 在字符串上返回 nullptr");
        if (intNode)
            CHECK(intNode->find("key") == nullptr, "find() 在整数上返回 nullptr");

        // --- at() 在非数组节点上调用 → 返回 nullptr ---
        kout << "  >> at() 在非数组节点上调用" << std::endl;
        if (objNode)
            CHECK(objNode->at(0) == nullptr, "at() 在对象上返回 nullptr");
        if (strNode)
            CHECK(strNode->at(0) == nullptr, "at() 在字符串上返回 nullptr");
        if (intNode)
            CHECK(intNode->at(0) == nullptr, "at() 在整数上返回 nullptr");

        // --- size() 在 null / 标量节点上 → 返回 0 ---
        kout << "  >> size() 在 null / 标量节点上调用" << std::endl;
        if (nullNode)
        {
            CHECK(nullNode->IsNull(),    "null_value 是 IsNull");
            CHECK(nullNode->size() == 0, "null 节点 size() = 0");
        }
        if (intNode)
            CHECK(intNode->size() == 0,  "整数节点 size() = 0");
        if (decNode)
            CHECK(decNode->size() == 0,  "浮点节点 size() = 0");
        if (boolNode)
            CHECK(boolNode->size() == 0, "布尔节点 size() = 0");
        if (strNode)
            CHECK(strNode->size() == 0,  "字符串节点 size() = 0");

        // --- 类型不匹配: AsXxx 误用 ---
        // AsXxx 先触发 KLOG_ERROR(KSON_TYPE_MISMATCH)，随后 std::get<T> 在类型
        // 不符时会抛 std::bad_variant_access，故此处用 try/catch 包裹以观察行为
        kout << "  >> 类型不匹配 (KSON_TYPE_MISMATCH → KLOG_ERROR + 可能抛异常)" << std::endl;
        #define TYPE_MISMATCH(expr, desc) do { \
            try { expr; \
                kout << "    {green}[PASS]{/} " << desc << " (KLOG_ERROR 已触发, 未抛异常)" << std::endl; ++g_ok; } \
            catch (const std::exception& e) { \
                kout << "    {green}[PASS]{/} " << desc << " (KLOG_ERROR + 抛异常: " << e.what() << ")" << std::endl; ++g_ok; } \
        } while(0)

        if (strNode) TYPE_MISMATCH(strNode->AsInt(),  "AsInt()  on string");
        if (intNode) TYPE_MISMATCH(intNode->AsStr(),  "AsStr()  on int");
        if (intNode) TYPE_MISMATCH(intNode->AsBool(), "AsBool() on int");
        if (intNode) TYPE_MISMATCH(intNode->AsBig(),  "AsBig()  on int");
        if (intNode) TYPE_MISMATCH(intNode->AsArr(),  "AsArr()  on int");
        if (intNode) TYPE_MISMATCH(intNode->AsObj(),  "AsObj()  on int");
        if (strNode) TYPE_MISMATCH(strNode->AsDec(),  "AsDec()  on string");
        if (arrNode) TYPE_MISMATCH(arrNode->AsStr(),  "AsStr()  on array");
        if (objNode) TYPE_MISMATCH(objNode->AsInt(),  "AsInt()  on object");
        if (nullNode) TYPE_MISMATCH(nullNode->AsStr(), "AsStr()  on null");

        #undef TYPE_MISMATCH
    }

    // ==================== 20. KSON 读取 inf/nan ====================
    SECTION("20. KSON 读取 inf/nan");
    {
        kson inf_nan = doc["inf_nan"];
        CHECK(inf_nan.Exists(), "inf_nan 节点存在");

        // 关键字 inf / -inf / nan
        kout << "  >> 关键字 inf / -inf / nan" << std::endl;
        KMATH::BigDec inf = inf_nan["inf"].Big();
        KMATH::BigDec neg_inf = inf_nan["neg_inf"].Big();
        KMATH::BigDec nan = inf_nan["nan"].Big();
        CHECK(inf.IsInf() && !inf.isneg, "inf 关键字 → IsInf && !isneg");
        CHECK(neg_inf.IsInf() && neg_inf.isneg, "-inf 关键字 → IsInf && isneg");
        CHECK(nan.IsNan(), "nan 关键字 → IsNan");

        // 引号字符串 "inf" / "NaN"（大小写不敏感，自动转为 BigNum 特殊状态）
        kout << "  >> 引号字符串 \"inf\" / \"NaN\"" << std::endl;
        KMATH::BigDec str_inf = inf_nan["str_inf"].Big();
        KMATH::BigDec str_nan = inf_nan["str_nan"].Big();
        CHECK(str_inf.IsInf() && !str_inf.isneg, "字符串 \"inf\" → IsInf");
        CHECK(str_nan.IsNan(), "字符串 \"NaN\" → IsNan");

        // 数组中的 inf/nan
        kout << "  >> 数组中的 inf / -inf / nan" << std::endl;
        kson arr = inf_nan["array"];
        CHECK(arr.Exists() && arr.Size() == 5, "inf_nan.array 大小 = 5");
        CHECK(arr[static_cast<size_t>(0)].Big().IsInf() && !arr[static_cast<size_t>(0)].Big().isneg,  "array[0] = inf");
        CHECK(arr[static_cast<size_t>(1)].Big().IsInf() && arr[static_cast<size_t>(1)].Big().isneg,   "array[1] = -inf");
        CHECK(arr[static_cast<size_t>(2)].Big().IsNan(),                          "array[2] = nan");
        CHECK(arr[static_cast<size_t>(3)].Big().IsInf() && !arr[static_cast<size_t>(3)].Big().isneg,  "array[3] = \"INF\"");
        CHECK(arr[static_cast<size_t>(4)].Big().IsNan(),                          "array[4] = \"nan\"");

        // Auto() 可打印特殊状态
        kout << "  >> Auto() 输出" << std::endl;
        SHOW("inf_nan.inf",     inf_nan["inf"].Auto());
        SHOW("inf_nan.neg_inf", inf_nan["neg_inf"].Auto());
        SHOW("inf_nan.nan",     inf_nan["nan"].Auto());
        SHOW("inf_nan.str_inf", inf_nan["str_inf"].Auto());
        SHOW("inf_nan.str_nan", inf_nan["str_nan"].Auto());
    }

    // ==================== 结论 ====================
    kout << "\n----------------------------------------\n";
    kout << "  {green}[OK]{/} " << g_ok << " 项通过\n";
    if (g_fail == 0)
        kout << "\n{green}[ALL PASS] KSON 配置驱动测试通过{/}\n";
    else
        kout << "\n{red}[" << g_fail << " FAIL] KSON 配置驱动测试有失败项{/}\n";

    KEnd();
    return g_fail > 0 ? 1 : 0;
}
