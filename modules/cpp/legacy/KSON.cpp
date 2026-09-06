
#include "modules/cpp/KF.hpp"

/**
 * @file KSON.cpp
 * @brief kson文件解析模块
 * @version 1.0.0
 * @date 2026-08-13
 * @author Git-1145
 */

namespace KF
{
    namespace KSON
    {
        constexpr char OBJ_BEGIN = '{';        // 对象开始
        constexpr char OBJ_END = '}';          // 对象结束
        constexpr char ARR_BEGIN = '[';        // 数组开始
        constexpr char ARR_END = ']';          // 数组结束
        constexpr char CHAR_NEG = '-';         // 负号
        constexpr char CHAR_POS = '+';         // 正号
        constexpr char CHAR_COMMA = ',';       // 逗号
        constexpr char CHAR_POINT = '.';       // 小数点
        constexpr char CHAR_COMMENT1 = '#';    // 单行注释
        constexpr char CHAR_SEPERATOR = ':';   // 分隔键值
        constexpr char CHAR_QUOTE1  = '\"';    // 字符串引号
        constexpr char CHAR_ESCAPE1  = '\\';   // 转义字符
        constexpr char CHAR_SCI_UP = 'E';      // 科学计数法大写
        constexpr char CHAR_SCI_LOW = 'e';     // 科学计数法小写        
        constexpr char CHAR_FORCE_BIG = 'B';   // 强制转换成大数类型（仅对普通整数/小数生效，科学计数法已自动转为 BigNum）
//---------------------------------------------UTILITY--------------------------------
        bool IsNumEnd(char c) noexcept //数字是否要结束了 碰到 逗号 ] } 结束
        {
            return c == CHAR_COMMA || c == ARR_END || c == OBJ_END;
        }
        bool IsStrEnd(char c) noexcept //字符串是否要结束了 碰到 " 结束
        {
            return c == CHAR_QUOTE1;
        }
//---------------------------------------------Node-----------------------------------
        Node::Node() noexcept : Data(std::monostate{}) {}  // 默认构造：空节点，类型为 kNull
        Node::Node(bool val) noexcept : Data(val) {}        // 从 bool 构造，类型为 kBool
        Node::Node(long long val) noexcept : Data(val) {}   // 从 long long 构造，类型为 kInt
        Node::Node(double val) noexcept : Data(val) {}      // 从 double 构造，类型为 kDec
        Node::Node(std::string val) noexcept : Data(std::move(val)) {}  // 从 string 构造，类型为 kStr
        Node::Node(std::vector<Node> val) : Data(std::move(val)) {}     // 从数组构造，类型为 kArr
        Node::Node(std::vector<std::pair<std::string, Node>> val) : Data(std::move(val)) {}  // 从对象构造，类型为 kObj
//---------------------------------------------IsSth-----------------------------------
        /// @brief 返回当前节点的类型
        /// @attention 使用 std::visit 遍历 variant，根据实际存储的类型返回对应的 NodeType
        NodeType Node::type() const noexcept
        {
            return std::visit([](auto&& arg) -> NodeType
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) return NodeType::kNull;
                else if constexpr (std::is_same_v<T, bool>) return NodeType::kBool;
                else if constexpr (std::is_same_v<T, long long>) return NodeType::kInt;
                else if constexpr (std::is_same_v<T, double>) return NodeType::kDec;
                else if constexpr (std::is_same_v<T, std::string>) return NodeType::kStr;
                else if constexpr (std::is_same_v<T, arr_t>) return NodeType::kArr;
                else return NodeType::kObj;
            }, Data);
        }
        /// @brief 判断是否为 null（空值）
        bool Node::IsNull()    const noexcept { return type() == NodeType::kNull; }
        /// @brief 判断是否为布尔值
        bool Node::IsBool()    const noexcept { return type() == NodeType::kBool; }
        /// @brief 判断是否为整数
        bool Node::IsInt()     const noexcept { return type() == NodeType::kInt; }
        /// @brief 判断是否为浮点数
        bool Node::IsDec()     const noexcept { return type() == NodeType::kDec; }
        /// @brief 判断是否为数字（整数或浮点数）
        bool Node::IsNumber()  const noexcept { return IsInt() || IsDec(); }
        /// @brief 判断是否为字符串
        bool Node::IsString()  const noexcept { return type() == NodeType::kStr; }
        /// @brief 判断是否为数组
        bool Node::IsArray()   const noexcept { return type() == NodeType::kArr; }
        /// @brief 判断是否为对象
        bool Node::IsObject()  const noexcept { return type() == NodeType::kObj; }
//---------------------------------------------AsSth-----------------------------------
        /// @brief 取布尔值
        bool Node::AsBool() const {
            if (!IsBool()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not boolean");
            return std::get<bool>(Data);
        }
        /// @brief 取整数值
        long long Node::AsInt() const {
            if (!IsInt()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not integer");
            return std::get<long long>(Data);
        }
        /// @brief 取浮点数值
        /// @attention 如果底层存储的是整数，会自动转换为 double
        double Node::AsDec() const {
            if (IsInt()) return static_cast<double>(AsInt());  // 整数可隐式转为浮点
            if (!IsDec()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not decimal");
            return std::get<double>(Data);
        }
        /// @brief 取字符串
        std::string_view Node::AsStr() const {
            if (!IsString()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not string");
            return std::get<std::string>(Data);
        }
        /// @brief 取数组引用
        const std::vector<Node>& Node::AsArr() const {
            if (!IsArray()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not array");
            return std::get<arr_t>(Data);
        }
        /// @brief 取对象引用
        const std::vector<std::pair<std::string, Node>>& Node::AsObj() const {
            if (!IsObject()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not object");
            return std::get<obj_t>(Data);
        }
        /// @brief 返回容器大小
        std::size_t Node::size() const
        {
            if (IsArray()) return AsArr().size();
            if (IsObject()) return AsObj().size();
            return 0;
        }
//---------------------------------------------find-----------------------------------
        /// @brief 在对象中按键查找
        /// @param key 要查找的键名
        /// @return 指向找到的 Node 的指针，未找到返回 nullptr
        const Node* Node::find(std::string_view key) const
        {
            if (!IsObject()) return nullptr;
            for (const auto& [k, v] : AsObj())
                if (k == key) return &v;
            return nullptr;  // 遍历完未找到
        }

        /// @brief 在数组中按下标查找
        /// @param index 数组下标
        /// @return 指向找到的 Node 的指针，越界返回 nullptr
        const Node* Node::at(std::size_t index) const {
            if (!IsArray()) return nullptr;
            const auto& arr = AsArr();
            if (index >= arr.size()) return nullptr;
            return &arr[index];
        }
//-------------------------------pathseg--------------------------------------------------
        /// @brief 构造一个键路径段（用于对象键访问）
        /// @param Key 键名字符串
        PathSeg::PathSeg(std::string Key) : key(std::move(Key)), index(0) {}

        /// @brief 构造一个索引路径段（用于数组下标访问）
        /// @param Index 数组下标
        PathSeg::PathSeg(std::size_t Index) : key(), index(Index) {}
//---------------------------------parse--------------------------------------------------
        /// @brief 解析一个kson文本
        class Parser
        {
            std::string_view str;
            size_t ReadPtr = 0;
            /// @brief 解析一个字符串字面量
            /// @return 字符串内容（已去掉首尾双引号，转义序列已解释为实际字符）
            std::string ParseStr()
            {
                if(ReadPtr >= str.size()) return "";
                bool IsEscape = false;//状态机 是否在转义模式
                bool AtLineStart = false;//当前是否在行首（换行后跳过缩进空白）
                size_t NumOfResize=0;
                size_t WritePtr=0;
                if(str[ReadPtr++] != CHAR_QUOTE1) KLOG_ERROR(KSON_PARSE_STRE,"");
                std::string res;
                while(ReadPtr < str.size())
                {
                    //容量不够,重新扩容 不用+=导致频繁的内存分配
                    if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                        res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                    if(IsEscape) //转义模式：把转义序列解释为实际字符
                    {
                        IsEscape = false;
                        char ec = str[ReadPtr++];
                        switch (ec)
                        {
                            case 'n':  res[WritePtr++] = '\n'; AtLineStart = true; break; //换行
                            case 't':  res[WritePtr++] = '\t'; break; //制表符
                            case 'r':  res[WritePtr++] = '\r'; break; //回车
                            case 'b':  res[WritePtr++] = '\b'; break; //退格
                            case '"':  res[WritePtr++] = '"';  break; //双引号
                            case '\\': res[WritePtr++] = '\\'; break; //反斜杠
                            default:
                                //未知转义序列：警告并原样保留该字符
                                KLOG_WARNING(KSON_PARSE_ESCAPE_SPECIAL,"");
                                res[WritePtr++] = ec;
                                break;
                        }
                        continue;
                    }
                    if(str[ReadPtr] == CHAR_ESCAPE1) //转义符，进入转义模式
                    {
                        IsEscape = true;
                        ReadPtr++;//消费反斜杠
                        continue;
                    }
                    /// @attention 退出窗口：未转义的双引号 → 字符串结束
                    else if(IsStrEnd(str[ReadPtr]))
                    {
                        ReadPtr++;//消费双引号
                        res.resize(WritePtr);//去掉多余的容量
                        return res;
                    }
                    // 行首缩进空白：跳过（跨行字符串的自动缩进不需要）
                    if(AtLineStart && (str[ReadPtr] == ' ' || str[ReadPtr] == '\t'))
                    {
                        ReadPtr++;
                        continue;
                    }
                    if(str[ReadPtr] == '\n') AtLineStart = true; //真实换行 → 下一行行首
                    else                      AtLineStart = false;
                    res[WritePtr++] = str[ReadPtr++]; //普通字符照抄
                }
                // 循环结束仍未匹配到双引号
                if (IsEscape) //反斜杠后直接到末尾，转义序列不完整
                    KLOG_FATAL(KSON_PARSE_UNFINISHED_ESCAPE,"");
                KLOG_ERROR(KSON_PARSE_STR_NOEND,"");
                return res;
            }
            /// @brief 解析一个数字
            /// @return 整数 小数 或未来支持的 科学计数 大数 等
            Node ParseNum()
            {
                if (ReadPtr >= str.size()) return Node(0LL);
                //if(str[ReadPtr++] != CHAR_NUM_QUOTE) KLOG_ERROR(KSON_PARSE_NUMOR,"");
                std::string res;
                bool dot = false,isneg=false,readnum=false; //是否有小数点 是否是负数 是否读到了数字
                size_t NumOfResize=0;
                size_t WritePtr=1;//从1开始，因为第0个字符是+或-
                res.resize(1);// 防止数字为空 导致res[0]失败
                size_t type = 0;  // 0:整数 1:小数 2:科学计数 3:大数
                auto ExitParse = [&]() -> Node
                {
                    res[0] = (isneg) ? CHAR_NEG : CHAR_POS;
                    res.resize(WritePtr);//去掉多余的容量
                    if(res.size() == 1) //如果数字为空
                        res="0";

                    // 大数/科学计数法/B 后缀均不支持，报错并返回 0
                    switch (type)
                    {
                        case 0: //整数
                        {
                            // 检查是否超出 int64_t 范围
                            std::string numStr = res.substr(1); // 去掉符号位
                            bool fitsInt64 = true;
                            if(numStr.size() > 19)
                                fitsInt64 = false;
                            else if(numStr.size() == 19)
                            {
                                std::string maxInt64 = "9223372036854775807";
                                if(numStr > maxInt64) fitsInt64 = false;
                            }
                            if(!fitsInt64)
                            {
                                // 超出 int64_t 范围 → 不支持大数，报错并返回 0
                                KLOG_ERROR(KSON_PARSE_NUMOR, res + " | integer overflow (big numbers not supported)");
                                return Node(0LL);
                            }
                            // stoll 在数字非法/溢出时会抛 invalid_argument / out_of_range，必须捕获
                            try { return Node(std::stoll(res)); }
                            catch (const std::exception& e)
                            {
                                KLOG_ERROR(KSON_PARSE_NUMOR, res + " | " + e.what());
                                return Node(0LL);
                            }
                        }
                        case 1: //小数 支持 .1 -1.
                        {
                            try { return Node(std::stod(res)); }
                            catch (const std::exception& e)
                            {
                                KLOG_ERROR(KSON_PARSE_NUMOR, res + " | " + e.what());
                                return Node(0.0);
                            }
                        }
                        case 2: //科学计数法 → 不支持，报错并返回 0
                        {
                            KLOG_ERROR(KSON_PARSE_NUMOR, res + " | scientific notation not supported");
                            return Node(0LL);
                        }
                        case 3: //大数（'B'后缀强制）→ 不支持，报错并返回 0
                        {
                            KLOG_ERROR(KSON_PARSE_NUMOR, res + " | big number suffix 'B' not supported");
                            return Node(0LL);
                        }
                        default: //暂不支持的数字类型
                            KLOG_ERROR(KSON_PARSE_NUM_USTYPE,"");
                            try { return Node(std::stoll(res)); }
                            catch (const std::exception& e)
                            {
                                KLOG_ERROR(KSON_PARSE_NUMOR, res + " | " + e.what());
                                return Node(0LL);
                            }
                    }
                };
                while(ReadPtr < str.size())
                {
                    //容量不够,重新扩容 不用+=导致频繁的内存分配
                    if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                        res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                    if(isdigit(static_cast<unsigned char>(str[ReadPtr]))) //如果是数字
                    {
                        readnum = true;
                        res[WritePtr++] = str[ReadPtr++];
                    }
                    else if(!readnum && (str[ReadPtr] == CHAR_NEG || str[ReadPtr] == CHAR_POS )) //如果是正负号
                    {
                        if(str[ReadPtr] == CHAR_NEG)
                            isneg ^= 1;
                        ReadPtr++;
                    }
                    else if(str[ReadPtr] == CHAR_POINT) //如果是小数点
                    {
                        if(dot) KLOG_WARNING(KSON_PARSE_MULPOINT,""); //有多个小数点
                        else
                        {
                            type = 1;
                            dot = true;
                            res[WritePtr++] = CHAR_POINT;
                        }
                        ReadPtr++;
                    }
                    else if(str[ReadPtr] == CHAR_SCI_LOW || str[ReadPtr] == CHAR_SCI_UP) //科学计数法
                    {
                        // 只有后面紧跟数字（或符号+数字）才算科学计数法
                        size_t peek = ReadPtr + 1;
                        if(peek < str.size() && (str[peek] == CHAR_NEG || str[peek] == CHAR_POS))
                            peek++;
                        if(peek < str.size() && isdigit(static_cast<unsigned char>(str[peek])))
                        {
                            type = 2;
                            res[WritePtr++] = str[ReadPtr++];
                            // 消费指数部分的符号位（e+5 或 e-5）
                            if(ReadPtr < str.size() && (str[ReadPtr] == CHAR_NEG || str[ReadPtr] == CHAR_POS))
                            {
                                if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                                    res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                                res[WritePtr++] = str[ReadPtr++];
                            }
                        }
                        else
                        {
                            // 不是科学计数法，当作普通字符跳过
                            KLOG_WARNING(KSON_PARSE_NUM_UE,"");
                            ReadPtr++;
                        }
                    }
                    else if(str[ReadPtr] == CHAR_FORCE_BIG) //强制 BigNum 后缀（科学计数法已自动 BigNum，不覆盖）
                    {
                        if(type != 2) type = 3;
                        ReadPtr++;
                    }
                    /// @attention 退出窗口
                    else if(IsNumEnd(str[ReadPtr]))
                    {
                        return ExitParse();
                    }
                    else //遇到不支持的非阿拉伯数字 发出警告
                    {
                        KLOG_WARNING(KSON_PARSE_NUM_UE,"");
                        ReadPtr++;
                    }
                }
                //KLOG_ERROR(KSON_PARSE_NUM_NOEND,"");
                return ExitParse();
                /// @attention 这种情况一般只会发生在根目录的最后一个键值对 此时while结束 读取的正好是完整的res
            }
            Node ParseVal()
            {
                if(ReadPtr >= str.size())
                {
                    KLOG_ERROR(KSON_PARSE_VAL_END,"");
                    return Node(0LL);
                }
                char c = str[ReadPtr];
                // 大小写不敏感关键字匹配（匹配成功则推进 ReadPtr）
                auto MatchKw = [&](std::string_view kw) -> bool
                {
                    const size_t n = kw.size();
                    if(ReadPtr + n > str.size()) return false;
                    for(size_t i = 0; i < n; i++)
                    {
                        char a = str[ReadPtr + i], b = kw[i];
                        if(a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
                        if(b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
                        if(a != b) return false;
                    }
                    ReadPtr += n;
                    return true;
                };
                switch (c)
                {
                    case CHAR_QUOTE1:
                    {
                        std::string s = ParseStr();
                        // 引号字符串 inf/-inf/nan（大小写不敏感）→ 转为 double 特殊值
                        std::string ls = s;
                        for(auto& ch : ls) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                        if(ls == "inf")  return Node(std::numeric_limits<double>::infinity());
                        if(ls == "-inf") return Node(-std::numeric_limits<double>::infinity());
                        if(ls == "nan")  return Node(std::numeric_limits<double>::quiet_NaN());
                        return Node(std::move(s));
                    }
                    case OBJ_BEGIN:
                        return Node(ParseObj());
                    case ARR_BEGIN:
                        return Node(ParseArr());
                    case 'i':
                    case 'I': // inf → 正无穷
                        if(MatchKw("inf"))
                            return Node(std::numeric_limits<double>::infinity());
                        KLOG_ERROR(KSON_PARSE_VAL_ERROR,"INF");
                        return Node(0LL);
                    case 't': //如果是Bool True
                        if(str.substr(ReadPtr,4) == "true")
                        {
                            ReadPtr += 4;
                            return Node(true);
                        }
                        KLOG_ERROR(KSON_PARSE_VAL_ERROR,"TRUE");
                        return Node(true);
                    case 'f': //如果是Bool False
                        if(str.substr(ReadPtr,5) == "false")
                        {
                            ReadPtr += 5;
                            return Node(false);
                        }
                        KLOG_ERROR(KSON_PARSE_VAL_ERROR,"FALSE");
                        return Node(false);
                    case 'n':
                    case 'N': // nan → 非数；否则按 null 处理
                        if(MatchKw("nan"))
                            return Node(std::numeric_limits<double>::quiet_NaN());
                        if(MatchKw("null"))
                            return Node();
                        KLOG_ERROR(KSON_PARSE_VAL_ERROR,"NULL");
                        return Node();
                    default:
                        // -inf → 负无穷
                        if(c == CHAR_NEG && MatchKw("-inf"))
                            return Node(-std::numeric_limits<double>::infinity());
                        // 数字（含正负号、小数点开头）交给 ParseNum 处理
                        if (std::isdigit(static_cast<unsigned char>(c)) || c == CHAR_NEG || c == CHAR_POS)
                            return ParseNum();
                        KLOG_ERROR(KSON_PARSE_VAL_ERROR,"Not supported");
                        return Node();
                }
            }
            /// @brief 解析一层数组
            Node ParseArr()
            {
                if(str[ReadPtr++] != ARR_BEGIN) KLOG_ERROR(KSON_PARSE_ARR_BEGIN,"");
                std::vector<Node> arr;
                if(ReadPtr < str.size() && str[ReadPtr] == ARR_END) //如果是空数组
                {
                    ReadPtr++;
                    return Node(std::move(arr));
                }
                while(ReadPtr < str.size())
                {
                    // 支持尾随逗号：如 [1,2,] 在逗号后紧跟 ]
                    if(str[ReadPtr] == ARR_END) { ReadPtr++; break; }
                    arr.push_back(ParseVal());
                    if(ReadPtr >= str.size()) break;
                    if(str[ReadPtr] == CHAR_COMMA) //如果还有下一组键值对或对象
                    {
                        ReadPtr++;
                        continue;
                    }
                    else if(str[ReadPtr] == ARR_END) //如果已经结束
                    {
                        ReadPtr++;
                        break;
                    }
                    else
                    {
                        // 非逗号、非 ] 的非预期字符（多由括号不匹配引起）：
                        // 必须推进 ReadPtr，否则原地踏步会死循环
                        KLOG_ERROR(KSON_PARSE_ARRUE,"");
                        ReadPtr++;
                    }
                }
                return Node(std::move(arr));
            }
            /// @brief 解析一层对象
            Node ParseObj()
            {
                if(str[ReadPtr++] != OBJ_BEGIN) KLOG_ERROR(KSON_PARSE_OBJ_BEGIN,"");
                std::vector<std::pair<std::string,Node>> obj;
                if(ReadPtr < str.size() && str[ReadPtr] == OBJ_END) //如果是空对象
                {
                    ReadPtr++;
                    return Node(obj);
                }
                while(ReadPtr < str.size())
                {
                    // 支持尾随逗号：如 {"a":1,} 在逗号后紧跟 }
                    if(str[ReadPtr] == OBJ_END) { ReadPtr++; break; }
                    if(str[ReadPtr] != CHAR_QUOTE1)//如果键不用引号包裹
                        KLOG_ERROR(KSON_PARSE_OBJ_KEY_QUOTE,"");
                    std::string key = ParseStr();
                    if(ReadPtr >= str.size() || str[ReadPtr] != CHAR_SEPERATOR) //如果键后面没有分隔符
                        KLOG_ERROR(KSON_PARSE_OBJ_SEPERATOR,"");
                    ReadPtr++;//消费分隔符
                    Node val = ParseVal();
                    bool IsFound = false;//是否有重复键 如果有 后覆盖前
                    for(auto& [k,v] : obj)
                    {
                        if(k == key) // 已经找到了所需键值对了 退出
                        {
                            v = std::move(val);
                            IsFound = true;
                            break;
                        }
                    }
                    if(!IsFound) obj.emplace_back(key,std::move(val)); //键值对不存在 则添加
                    if(ReadPtr >= str.size()) break;
                    if(str[ReadPtr] == CHAR_COMMA) //如果还有下一组键值对或对象
                    {
                        ReadPtr++;
                        continue;
                    }
                    else if(str[ReadPtr] == OBJ_END) //如果已经结束
                    {
                        ReadPtr++;
                        break;
                    }
                    else
                    {
                        // 非逗号、非 } 的非预期字符（多由括号不匹配引起）：
                        // 必须推进 ReadPtr，否则原地踏步会死循环
                        KLOG_ERROR(KSON_PARSE_OBJUE,"");
                        ReadPtr++;
                    }
                }
                return Node(std::move(obj));
            }
            /// @brief 窥探顶层是否为隐式对象（"key": value 形式，无外层 {}）
            /// @note  从 ReadPtr 起，若以 " 开头，且该字符串的闭合引号后紧跟 ':'，
            ///        则判定为隐式对象。不移动 ReadPtr。
            bool PeekIsImplicitObj() const
            {
                if (ReadPtr >= str.size() || str[ReadPtr] != CHAR_QUOTE1) return false;
                size_t i = ReadPtr + 1;
                bool isEscape = false;
                while (i < str.size())
                {
                    char c = str[i];
                    if (isEscape) { isEscape = false; i++; continue; } //跳过被转义的字符
                    if (c == CHAR_ESCAPE1) { isEscape = true; i++; continue; }
                    if (c == CHAR_QUOTE1) { i++; break; } //找到未转义的闭合引号
                    i++;
                }
                // 闭合引号后（Preprocess 已去除外层空白）紧跟 ':' 即为隐式对象
                return (i < str.size() && str[i] == CHAR_SEPERATOR);
            }
            /// @brief 解析顶层隐式对象（无外层 {}，直接 "key": value, "key2": value2 形式）
            /// @note  用于配置文件场景：顶层就是一组键值对，直到 EOF 结束。
            ///        逻辑与 ParseObj 一致，区别在于没有起始 { 与终止 }，仅遇 EOF 收尾。
            Node ParseImplicitObj()
            {
                std::vector<std::pair<std::string,Node>> obj;
                while(ReadPtr < str.size())
                {
                    if(str[ReadPtr] != CHAR_QUOTE1) //键必须用双引号包裹
                        KLOG_ERROR(KSON_PARSE_OBJ_KEY_QUOTE,"");
                    std::string key = ParseStr();
                    if(ReadPtr >= str.size() || str[ReadPtr] != CHAR_SEPERATOR) //键后缺少冒号分隔符
                        KLOG_ERROR(KSON_PARSE_OBJ_SEPERATOR,"");
                    ReadPtr++;//消费分隔符
                    Node val = ParseVal();
                    bool IsFound = false;//重复键后覆盖前
                    for(auto& [k,v] : obj)
                    {
                        if(k == key) { v = std::move(val); IsFound = true; break; }
                    }
                    if(!IsFound) obj.emplace_back(key,std::move(val));
                    if(ReadPtr >= str.size()) break;
                    if(str[ReadPtr] == CHAR_COMMA) { ReadPtr++; continue; } //还有下一组键值对
                    else { KLOG_ERROR(KSON_PARSE_OBJUE,""); ReadPtr++; } //非预期字符，推进避免死循环
                }
                return Node(std::move(obj));
            }
            public:
                explicit Parser(std::string_view str) : str(std::move(str)) {}
                Node Parse()
                {
                    // 顶层支持两种形式：
                    //   1) 单个值（对象/数组/字符串/数字/布尔/null）
                    //   2) 隐式对象：直接 "key": value 形式（无外层 {}），便于做配置文件
                    //      例如 cfg.txt 顶层就是 "test": { ... }，没有外层花括号
                    if (PeekIsImplicitObj())
                        return ParseImplicitObj();
                    Node root = ParseVal();
                    /// @brief 检查是否还有未消费的字符
                    if(ReadPtr < str.size()) KLOG_WARNING(KSON_PARSE_TRAIL,"");
                    return root;
                }
        };
//--------------------------------NodePtr----------------------------------------------
        NodePtr::NodePtr() noexcept = default;

        NodePtr::NodePtr(std::shared_ptr<Node> r) noexcept
            : root_(std::move(r)) {}

        NodePtr::NodePtr(std::shared_ptr<Node> r, std::vector<PathSeg> p) noexcept
            : root_(std::move(r)), path_(std::move(p)) {}

        const Node* NodePtr::ResolvePath(const std::vector<PathSeg>& path) const {
            if (!root_) return nullptr;
            const Node* cur = root_.get();
            for (const auto& seg : path) {
                if (!cur) return nullptr;
                cur = seg.key.empty() ? cur->at(seg.index) : cur->find(seg.key);
            }
            return cur;
        }

        const Node* NodePtr::TryResolve() const {
            if (!root_) return nullptr;
            if (cached_) return cached_;
            cached_ = ResolvePath(path_);
            return cached_;
        }

        const Node* NodePtr::Resolve() const {
            const Node* n = TryResolve();
            if (!n) KLOG_FATAL(UNKNOWN,"path not found");
            return n;
        }

        NodePtr NodePtr::operator[](std::string_view key) const {
            auto p = path_;
            p.emplace_back(std::string(key));
            return NodePtr(root_, std::move(p));
        }

        NodePtr NodePtr::operator[](std::size_t i) const {
            auto p = path_;
            p.emplace_back(i);
            return NodePtr(root_, std::move(p));
        }

        NodePtr NodePtr::operator[](const char* key) const {
            return (*this)[std::string_view(key)];
        }

        // 静态解析工厂
        NodePtr NodePtr::Parse(std::string_view text) {
            auto root = std::make_shared<Node>(Parser(text).Parse());
            return NodePtr(root);
        }

        NodePtr NodePtr::ParseFile(std::string_view filepath) {
            std::string content = KFIO::ReadFileRaw(filepath);
            return Parse(content);
        }

        // 代理方法
        std::string NodePtr::Str()  const { return std::string(Resolve()->AsStr()); }
        long long   NodePtr::Int()  const { return Resolve()->AsInt(); }
        double      NodePtr::Dec()  const { return Resolve()->AsDec(); }
        bool        NodePtr::Bool() const { return Resolve()->AsBool(); }
        std::size_t NodePtr::Size() const { return Resolve()->size(); }
        bool        NodePtr::Exists() const { return TryResolve() != nullptr; }

        /// @brief 递归将 Node 转换为可打印字符串（Auto 的核心实现）
        /// @param n 指向节点的指针，nullptr 时返回 "null"
        /// @return 按类型自动转换的字符串：
        ///         kNull→"null"  kBool→"true"/"false"  kInt→整数字串
        ///         kDec→浮点字串  kStr→字符串原文（不加引号）
        ///         kArr→[e1, e2, ...]   kObj→{"key": val, ...}
        static std::string NodeAutoString(const Node* n)
        {
            if (!n) return "null";
            switch (n->type())
            {
                case NodeType::kNull:
                    return "null";
                case NodeType::kBool:
                    return n->AsBool() ? "true" : "false";
                case NodeType::kInt:
                    return std::to_string(n->AsInt());
                case NodeType::kDec:
                {
                    double d = n->AsDec();
                    // inf/-inf/nan 特殊值 → 固定文本
                    if (std::isinf(d)) return d < 0 ? "-inf" : "inf";
                    if (std::isnan(d)) return "nan";
                    // 用 ostringstream + setprecision(15) 保留完整精度，
                    // 再去掉尾部的多余零，避免 std::to_string 只显示 6 位小数
                    std::ostringstream oss;
                    oss << std::setprecision(15) << d;
                    std::string s = oss.str();
                    // 去掉小数点后多余的尾零：如 1.114500 → 1.1145
                    if (s.find('.') != std::string::npos)
                    {
                        size_t last = s.find_last_not_of('0');
                        if (s[last] == '.') last--; // 小数点后全零则保留一位：1.0
                        s.erase(last + 1);
                    }
                    return s;
                }
                case NodeType::kStr:
                    return std::string(n->AsStr());
                case NodeType::kArr:
                {
                    // 递归序列化数组：[elem0, elem1, ...]
                    std::string res = "[";
                    const auto& arr = n->AsArr();
                    for (std::size_t i = 0; i < arr.size(); ++i)
                    {
                        if (i) res += ", ";
                        res += NodeAutoString(&arr[i]);
                    }
                    res += "]";
                    return res;
                }
                case NodeType::kObj:
                {
                    // 递归序列化对象：{"key": val, "key2": val2}
                    std::string res = "{";
                    const auto& obj = n->AsObj();
                    for (std::size_t i = 0; i < obj.size(); ++i)
                    {
                        if (i) res += ", ";
                        res += "\"" + obj[i].first + "\": " + NodeAutoString(&obj[i].second);
                    }
                    res += "}";
                    return res;
                }
            }
            return "null";
        }

        std::string NodePtr::Auto() const
        {
            // 用 TryResolve 而非 Resolve，路径未找到时返回 "null" 而非 Fatal
            return NodeAutoString(TryResolve());
        }

        std::size_t NodePtr::size() const { return Size(); }
//--------------------------------preprocess----------------------------------------------
        /// @brief 文件读取并处理
        std::string Preprocess(std::string raw)
        {
            std::string res;
            res.resize(raw.size());
            size_t WriteIndex = 0, ReadIndex = 0;  // 读到哪 写到哪
            // 跳过 UTF-8 BOM（EF BB BF）：带 BOM 的 kson 文件首字符会被误判为非法值
            if(raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
               static_cast<unsigned char>(raw[1]) == 0xBB &&
               static_cast<unsigned char>(raw[2]) == 0xBF)
                ReadIndex = 3;
            enum State { Normal, InString, InEscape} state = Normal;  // 状态机 同时只能处于一种状态
            for(;ReadIndex < raw.size();)
            {
                char c = raw[ReadIndex];
                switch (state)
                {
                    case Normal:
                        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\b')
                            ReadIndex++;//跳过空白符号
                        else if(c == CHAR_COMMENT1)//如果是注释行
                        {
                            auto end = raw.find('\n', ReadIndex);//跳过这行
                            ReadIndex = (end == std::string::npos) ? raw.size() : end;
                            ReadIndex++;
                        }
                        else if(c == CHAR_QUOTE1)//如果是引号
                        {
                            state = InString;
                            res[WriteIndex++] = c;
                            ReadIndex++;
                        }
                        else
                        {
                            res[WriteIndex++] = c;
                            ReadIndex++;
                        }
                        break;
                    case InString:
                        if(c == CHAR_ESCAPE1) //转义符：保留反斜杠，进入转义态以跳过下一字符的边界判定
                        {
                            state = InEscape;
                            res[WriteIndex++] = c;
                            ReadIndex++;
                        }
                        else if(c == CHAR_QUOTE1) //未转义的双引号 → 字符串结束
                        {
                            state = Normal;
                            res[WriteIndex++] = c;
                            ReadIndex++;
                        }
                        else
                        {
                            res[WriteIndex++] = c;
                            ReadIndex++;
                        }
                        break;
                    case InEscape:
                        res[WriteIndex++] = c;
                        state = InString;
                        ReadIndex++;
                        break;
                }
            }
            res.resize(WriteIndex);
            return res;
        }
        kson read(std::string_view processed)
        {
            return NodePtr::Parse(processed);
        }
        kson ReadKsonFile(std::string_view filename)
        {
            EnableVT100();
            return read(KSON::Preprocess(KFIO::ReadFileRaw(filename)));
        }
    }
}