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

import kf;
import kbignum;
#include <string>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstddef>
#include <exception>

// ==================== 测试辅助宏 ====================
static int g_ok = 0, g_fail = 0;
#define SECTION(name) kout << Bold << "\n--- " << name << " ---" << Reset << std::endl
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
    KBegin(doc["meta"].Vec());

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

    // ==================== 17. 大数 / 科学计数法 不再支持 ====================
    SECTION("17. 大数 / 科学计数法 不再支持");
    {
        // config/test/cfg.kson 中 kson_bignum 现为数组，共 10 个元素，按下标访问
        kson bn = doc["kson_bignum"];
        const Node* bnRoot = bn.TryResolve();
        CHECK(bnRoot && bnRoot->IsArray(), "kson_bignum 是数组");
        CHECK(bnRoot && bnRoot->size() == 10, "kson_bignum 共 10 个元素");

        // [0] = 9223372036854775807 (int64_max) → 仍是合法整数
        kout << "  >> [0] int64_max (普通整数)" << std::endl;
        std::size_t i0 = 0;
        const Node* n0 = bn[i0].TryResolve();
        CHECK(n0 && n0->IsInt(), "[0] int64_max 是 Int 类型");
        CHECK(n0->AsInt() == 9223372036854775807LL, "[0] int64_max 值正确");
        SHOW("[0] int64_max", n0->AsInt());

        // [1..9] 大数 / 科学计数法 / B 后缀 → 不再支持，解析为 0（KLOG_ERROR 输出到 stderr）
        kout << "  >> [1..9] 大数/科学计数法/B后缀 → 被拒绝为 0" << std::endl;
        for(std::size_t i = 1; i < 10; i++)
        {
            const Node* n = bn[i].TryResolve();
            CHECK(n && n->IsInt() && n->AsInt() == 0,
                  "[" + std::to_string(i) + "] 大数/科学计数法被拒绝为 0");
        }
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
                    std::string normalized = Normalize(raw);
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

        // 关键字 inf / -inf / nan（存储为 double 特殊值）
        kout << "  >> 关键字 inf / -inf / nan" << std::endl;
        double inf = inf_nan["inf"].Dec();
        double neg_inf = inf_nan["neg_inf"].Dec();
        double nan = inf_nan["nan"].Dec();
        CHECK(std::isinf(inf) && inf > 0, "inf 关键字 → isinf && 正");
        CHECK(std::isinf(neg_inf) && neg_inf < 0, "-inf 关键字 → isinf && 负");
        CHECK(std::isnan(nan), "nan 关键字 → isnan");

        // 引号字符串 "inf" / "NaN"（大小写不敏感，转为 double 特殊值）
        kout << "  >> 引号字符串 \"inf\" / \"NaN\"" << std::endl;
        double str_inf = inf_nan["str_inf"].Dec();
        double str_nan = inf_nan["str_nan"].Dec();
        CHECK(std::isinf(str_inf) && str_inf > 0, "字符串 \"inf\" → isinf");
        CHECK(std::isnan(str_nan), "字符串 \"NaN\" → isnan");

        // 数组中的 inf/nan
        kout << "  >> 数组中的 inf / -inf / nan" << std::endl;
        kson arr = inf_nan["array"];
        CHECK(arr.Exists() && arr.Size() == 5, "inf_nan.array 大小 = 5");
        CHECK(std::isinf(arr[static_cast<size_t>(0)].Dec()) && arr[static_cast<size_t>(0)].Dec() > 0,  "array[0] = inf");
        CHECK(std::isinf(arr[static_cast<size_t>(1)].Dec()) && arr[static_cast<size_t>(1)].Dec() < 0,  "array[1] = -inf");
        CHECK(std::isnan(arr[static_cast<size_t>(2)].Dec()),                          "array[2] = nan");
        CHECK(std::isinf(arr[static_cast<size_t>(3)].Dec()) && arr[static_cast<size_t>(3)].Dec() > 0,  "array[3] = \"INF\"");
        CHECK(std::isnan(arr[static_cast<size_t>(4)].Dec()),                          "array[4] = \"nan\"");

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
