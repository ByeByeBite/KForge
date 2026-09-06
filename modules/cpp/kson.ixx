module;
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <memory>
#include <utility>
#include <cctype>
#include <limits>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <windows.h>

// 日志宏：自动捕获调用位置（本模块内使用，位于  内）
#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module kson;

import klogger;

export
{
    constexpr size_t DEFAULT_RESIZE_STR_LEN = 64; // 默认KSON中字符串的分配长度 (超过这个长度会再次扩容)

    /////////////////////////////////////////////////////////
    // MazeCell 枚举 + 迷宫字符常量（KFIO 并入 kson）
    /////////////////////////////////////////////////////////
    constexpr char MAZE_WALL  = 'W';   // 墙字符
    constexpr char MAZE_PATH  = 'P';   // 通路字符
    constexpr char MAZE_START = 'S';   // 起点字符
    constexpr char MAZE_END   = 'E';   // 终点字符
    enum class MazeCell
    {
        WALL,      // 墙
        PASSABLE,  // 可通行
        VISITED,   // 已访问
        START,     // 起点
        END,       // 终点
        PATH       // 最终路径
    };

    /////////////////////////////////////////////////////////
    // KSON 节点
    /////////////////////////////////////////////////////////
    class Node;
    enum class NodeType // 节点
    {
        kInt, // Integer 整数
        kDec, // Decimal 浮点数（含 inf/-inf/nan）
        kStr, // String 字符串
        kBool, // Boolean 布尔值
        kArr, // Array 数组
        kObj, // Object 对象
        kNull, // Null 空值
    };
    class Node
    {
        public:
            using arr_t = std::vector<Node>;
            using obj_t = std::vector<std::pair<std::string, Node>>;
            using storage_t = std::variant<std::monostate, bool, std::string, double, long long, arr_t, obj_t>;

            Node() noexcept;
            explicit Node(bool val) noexcept;
            explicit Node(long long val) noexcept;
            explicit Node(double val) noexcept;
            explicit Node(std::string val) noexcept;
            explicit Node(arr_t val);
            explicit Node(obj_t val);

            NodeType type()  const noexcept;
            bool IsNull()    const noexcept;
            bool IsBool()    const noexcept;
            bool IsInt()     const noexcept;
            bool IsDec()     const noexcept;
            bool IsNumber()  const noexcept;
            bool IsString()  const noexcept;
            bool IsArray()   const noexcept;
            bool IsObject()  const noexcept;

            bool             AsBool()   const;
            long long        AsInt()    const;
            double           AsDec() const;
            std::string_view AsStr() const;
            const arr_t&     AsArr()  const;
            const obj_t&     AsObj() const;

            std::size_t size() const;
            const Node* find(std::string_view key) const;
            const Node* at(std::size_t index)      const;

        private:
            storage_t Data;
    };

    struct PathSeg
    {
        std::string key;
        std::size_t index;
        PathSeg(std::string Key);
        PathSeg(std::size_t Index);
    };

    class NodePtr
    {
        public:
            NodePtr() noexcept;
            explicit NodePtr(std::shared_ptr<Node> root) noexcept;
            NodePtr(std::shared_ptr<Node> root, std::vector<PathSeg> path) noexcept;

            static NodePtr Parse(std::string_view text);
            static NodePtr ParseFile(std::string_view filepath);

            NodePtr operator[](std::string_view key) const;
            NodePtr operator[](std::size_t index) const;
            NodePtr operator[](const char* key) const;

            const Node* TryResolve() const;
            const Node* Resolve() const;

            std::string Str() const;
            long long Int() const;
            double Dec() const;
            bool Bool() const;
            std::size_t Size() const;
            std::size_t size() const;
            bool Exists() const;
            std::string Auto() const;

            /// @brief 把数组/对象节点转为字符串向量（数组取元素，对象取值），供 KCLI 菜单等使用
            std::vector<std::string> Vec() const;

        private:
            std::shared_ptr<Node> root_;
            std::vector<PathSeg> path_;
            mutable const Node* cached_ = nullptr;
            const Node* ResolvePath(const std::vector<PathSeg>& path) const;
    };
    using kson = NodePtr;

    /// @brief 读取文件并解析
    std::string Preprocess(std::string raw);
    kson read(std::string_view processed);
    kson ReadKsonFile(std::string_view filename);

    /////////////////////////////////////////////////////////
    // 文件读取（KFIO 并入）
    /////////////////////////////////////////////////////////
    std::string ReadFileRaw(std::string_view filepath);
    std::vector<std::vector<MazeCell>> ReadMaze(std::string_view filepath, std::string_view maze_key = "small");
}



    //===============================================================
    //  KSON 解析辅助常量
    //===============================================================
    namespace
    {
        constexpr char OBJ_BEGIN = '{';
        constexpr char OBJ_END = '}';
        constexpr char ARR_BEGIN = '[';
        constexpr char ARR_END = ']';
        constexpr char CHAR_NEG = '-';
        constexpr char CHAR_POS = '+';
        constexpr char CHAR_COMMA = ',';
        constexpr char CHAR_POINT = '.';
        constexpr char CHAR_COMMENT1 = '#';
        constexpr char CHAR_SEPERATOR = ':';
        constexpr char CHAR_QUOTE1 = '"';
        constexpr char CHAR_ESCAPE1 = '\\';
        constexpr char CHAR_SCI_UP = 'E';
        constexpr char CHAR_SCI_LOW = 'e';
        constexpr char CHAR_FORCE_BIG = 'B';

        bool IsNumEnd(char c) noexcept { return c == CHAR_COMMA || c == ARR_END || c == OBJ_END; }
        bool IsStrEnd(char c) noexcept { return c == CHAR_QUOTE1; }
    }

    //---------------------------------------------Node-----------------------------------
    Node::Node() noexcept : Data(std::monostate{}) {}
    Node::Node(bool val) noexcept : Data(val) {}
    Node::Node(long long val) noexcept : Data(val) {}
    Node::Node(double val) noexcept : Data(val) {}
    Node::Node(std::string val) noexcept : Data(std::move(val)) {}
    Node::Node(arr_t val) : Data(std::move(val)) {}
    Node::Node(obj_t val) : Data(std::move(val)) {}

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
    bool Node::IsNull()    const noexcept { return type() == NodeType::kNull; }
    bool Node::IsBool()    const noexcept { return type() == NodeType::kBool; }
    bool Node::IsInt()     const noexcept { return type() == NodeType::kInt; }
    bool Node::IsDec()     const noexcept { return type() == NodeType::kDec; }
    bool Node::IsNumber()  const noexcept { return IsInt() || IsDec(); }
    bool Node::IsString()  const noexcept { return type() == NodeType::kStr; }
    bool Node::IsArray()   const noexcept { return type() == NodeType::kArr; }
    bool Node::IsObject()  const noexcept { return type() == NodeType::kObj; }

    bool Node::AsBool() const {
        if (!IsBool()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not boolean");
        return std::get<bool>(Data);
    }
    long long Node::AsInt() const {
        if (!IsInt()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not integer");
        return std::get<long long>(Data);
    }
    double Node::AsDec() const {
        if (IsInt()) return static_cast<double>(AsInt());
        if (!IsDec()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not decimal");
        return std::get<double>(Data);
    }
    std::string_view Node::AsStr() const {
        if (!IsString()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not string");
        return std::get<std::string>(Data);
    }
    const Node::arr_t& Node::AsArr() const {
        if (!IsArray()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not array");
        return std::get<arr_t>(Data);
    }
    const Node::obj_t& Node::AsObj() const {
        if (!IsObject()) KLOG_ERROR(KSON_TYPE_MISMATCH, "Node is not object");
        return std::get<obj_t>(Data);
    }
    std::size_t Node::size() const
    {
        if (IsArray()) return AsArr().size();
        if (IsObject()) return AsObj().size();
        return 0;
    }
    const Node* Node::find(std::string_view key) const
    {
        if (!IsObject()) return nullptr;
        for (const auto& [k, v] : AsObj())
            if (k == key) return &v;
        return nullptr;
    }
    const Node* Node::at(std::size_t index) const {
        if (!IsArray()) return nullptr;
        const auto& arr = AsArr();
        if (index >= arr.size()) return nullptr;
        return &arr[index];
    }

    PathSeg::PathSeg(std::string Key) : key(std::move(Key)), index(0) {}
    PathSeg::PathSeg(std::size_t Index) : key(), index(Index) {}

    //---------------------------------parse--------------------------------------------------
    class Parser
    {
        std::string_view str;
        size_t ReadPtr = 0;
        std::string ParseStr()
        {
            if(ReadPtr >= str.size()) return "";
            bool IsEscape = false;
            bool AtLineStart = false;
            size_t NumOfResize=0;
            size_t WritePtr=0;
            if(str[ReadPtr++] != CHAR_QUOTE1) KLOG_ERROR(KSON_PARSE_STRE,"");
            std::string res;
            while(ReadPtr < str.size())
            {
                if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                    res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                if(IsEscape)
                {
                    IsEscape = false;
                    char ec = str[ReadPtr++];
                    switch (ec)
                    {
                        case 'n':  res[WritePtr++] = '\n'; AtLineStart = true; break;
                        case 't':  res[WritePtr++] = '\t'; break;
                        case 'r':  res[WritePtr++] = '\r'; break;
                        case 'b':  res[WritePtr++] = '\b'; break;
                        case '"':  res[WritePtr++] = '"';  break;
                        case '\\': res[WritePtr++] = '\\'; break;
                        default:
                            KLOG_WARNING(KSON_PARSE_ESCAPE_SPECIAL,"");
                            res[WritePtr++] = ec;
                            break;
                    }
                    continue;
                }
                if(str[ReadPtr] == CHAR_ESCAPE1)
                {
                    IsEscape = true;
                    ReadPtr++;
                    continue;
                }
                else if(IsStrEnd(str[ReadPtr]))
                {
                    ReadPtr++;
                    res.resize(WritePtr);
                    return res;
                }
                if(AtLineStart && (str[ReadPtr] == ' ' || str[ReadPtr] == '\t'))
                {
                    ReadPtr++;
                    continue;
                }
                if(str[ReadPtr] == '\n') AtLineStart = true;
                else                      AtLineStart = false;
                res[WritePtr++] = str[ReadPtr++];
            }
            if (IsEscape)
                KLOG_FATAL(KSON_PARSE_UNFINISHED_ESCAPE,"");
            KLOG_ERROR(KSON_PARSE_STR_NOEND,"");
            return res;
        }
        Node ParseNum()
        {
            if (ReadPtr >= str.size()) return Node(0LL);
            std::string res;
            bool dot = false,isneg=false,readnum=false;
            size_t NumOfResize=0;
            size_t WritePtr=1;
            res.resize(1);
            size_t type = 0;
            auto ExitParse = [&]() -> Node
            {
                res[0] = (isneg) ? CHAR_NEG : CHAR_POS;
                res.resize(WritePtr);
                if(res.size() == 1)
                    res="0";
                switch (type)
                {
                    case 0:
                    {
                        std::string numStr = res.substr(1);
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
                            KLOG_ERROR(KSON_PARSE_NUMOR, res + " | integer overflow (big numbers not supported)");
                            return Node(0LL);
                        }
                        try { return Node(std::stoll(res)); }
                        catch (const std::exception& e)
                        {
                            KLOG_ERROR(KSON_PARSE_NUMOR, res + " | " + e.what());
                            return Node(0LL);
                        }
                    }
                    case 1:
                    {
                        try { return Node(std::stod(res)); }
                        catch (const std::exception& e)
                        {
                            KLOG_ERROR(KSON_PARSE_NUMOR, res + " | " + e.what());
                            return Node(0.0);
                        }
                    }
                    case 2:
                    {
                        KLOG_ERROR(KSON_PARSE_NUMOR, res + " | scientific notation not supported");
                        return Node(0LL);
                    }
                    case 3:
                    {
                        KLOG_ERROR(KSON_PARSE_NUMOR, res + " | big number suffix 'B' not supported");
                        return Node(0LL);
                    }
                    default:
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
                if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                    res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                if(isdigit(static_cast<unsigned char>(str[ReadPtr])))
                {
                    readnum = true;
                    res[WritePtr++] = str[ReadPtr++];
                }
                else if(!readnum && (str[ReadPtr] == CHAR_NEG || str[ReadPtr] == CHAR_POS ))
                {
                    if(str[ReadPtr] == CHAR_NEG)
                        isneg ^= 1;
                    ReadPtr++;
                }
                else if(str[ReadPtr] == CHAR_POINT)
                {
                    if(dot) KLOG_WARNING(KSON_PARSE_MULPOINT,"");
                    else
                    {
                        type = 1;
                        dot = true;
                        res[WritePtr++] = CHAR_POINT;
                    }
                    ReadPtr++;
                }
                else if(str[ReadPtr] == CHAR_SCI_LOW || str[ReadPtr] == CHAR_SCI_UP)
                {
                    size_t peek = ReadPtr + 1;
                    if(peek < str.size() && (str[peek] == CHAR_NEG || str[peek] == CHAR_POS))
                        peek++;
                    if(peek < str.size() && isdigit(static_cast<unsigned char>(str[peek])))
                    {
                        type = 2;
                        res[WritePtr++] = str[ReadPtr++];
                        if(ReadPtr < str.size() && (str[ReadPtr] == CHAR_NEG || str[ReadPtr] == CHAR_POS))
                        {
                            if(WritePtr >= DEFAULT_RESIZE_STR_LEN * NumOfResize)
                                res.resize(DEFAULT_RESIZE_STR_LEN * ++NumOfResize);
                            res[WritePtr++] = str[ReadPtr++];
                        }
                    }
                    else
                    {
                        KLOG_WARNING(KSON_PARSE_NUM_UE,"");
                        ReadPtr++;
                    }
                }
                else if(str[ReadPtr] == CHAR_FORCE_BIG)
                {
                    if(type != 2) type = 3;
                    ReadPtr++;
                }
                else if(IsNumEnd(str[ReadPtr]))
                {
                    return ExitParse();
                }
                else
                {
                    KLOG_WARNING(KSON_PARSE_NUM_UE,"");
                    ReadPtr++;
                }
            }
            return ExitParse();
        }
        Node ParseVal()
        {
            if(ReadPtr >= str.size())
            {
                KLOG_ERROR(KSON_PARSE_VAL_END,"");
                return Node(0LL);
            }
            char c = str[ReadPtr];
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
                    std::string ls = s;
                    for(auto& ch : ls) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    if(ls == "inf")  return Node(std::numeric_limits<double>::infinity());
                    if(ls == "-inf") return Node(-std::numeric_limits<double>::infinity());
                    if(ls == "nan")  return Node(std::numeric_limits<double>::quiet_NaN());
                    return Node(std::move(s));
                }
                case OBJ_BEGIN: return Node(ParseObj());
                case ARR_BEGIN: return Node(ParseArr());
                case 'i':
                case 'I':
                    if(MatchKw("inf")) return Node(std::numeric_limits<double>::infinity());
                    KLOG_ERROR(KSON_PARSE_VAL_ERROR,"INF");
                    return Node(0LL);
                case 't':
                    if(str.substr(ReadPtr,4) == "true") { ReadPtr += 4; return Node(true); }
                    KLOG_ERROR(KSON_PARSE_VAL_ERROR,"TRUE");
                    return Node(true);
                case 'f':
                    if(str.substr(ReadPtr,5) == "false") { ReadPtr += 5; return Node(false); }
                    KLOG_ERROR(KSON_PARSE_VAL_ERROR,"FALSE");
                    return Node(false);
                case 'n':
                case 'N':
                    if(MatchKw("nan")) return Node(std::numeric_limits<double>::quiet_NaN());
                    if(MatchKw("null")) return Node();
                    KLOG_ERROR(KSON_PARSE_VAL_ERROR,"NULL");
                    return Node();
                default:
                    if(c == CHAR_NEG && MatchKw("-inf"))
                        return Node(-std::numeric_limits<double>::infinity());
                    if (std::isdigit(static_cast<unsigned char>(c)) || c == CHAR_NEG || c == CHAR_POS)
                        return ParseNum();
                    KLOG_ERROR(KSON_PARSE_VAL_ERROR,"Not supported");
                    return Node();
            }
        }
        Node ParseArr()
        {
            if(str[ReadPtr++] != ARR_BEGIN) KLOG_ERROR(KSON_PARSE_ARR_BEGIN,"");
            std::vector<Node> arr;
            if(ReadPtr < str.size() && str[ReadPtr] == ARR_END) { ReadPtr++; return Node(std::move(arr)); }
            while(ReadPtr < str.size())
            {
                if(str[ReadPtr] == ARR_END) { ReadPtr++; break; }
                arr.push_back(ParseVal());
                if(ReadPtr >= str.size()) break;
                if(str[ReadPtr] == CHAR_COMMA) { ReadPtr++; continue; }
                else if(str[ReadPtr] == ARR_END) { ReadPtr++; break; }
                else
                {
                    KLOG_ERROR(KSON_PARSE_ARRUE,"");
                    ReadPtr++;
                }
            }
            return Node(std::move(arr));
        }
        Node ParseObj()
        {
            if(str[ReadPtr++] != OBJ_BEGIN) KLOG_ERROR(KSON_PARSE_OBJ_BEGIN,"");
            std::vector<std::pair<std::string,Node>> obj;
            if(ReadPtr < str.size() && str[ReadPtr] == OBJ_END) { ReadPtr++; return Node(obj); }
            while(ReadPtr < str.size())
            {
                if(str[ReadPtr] == OBJ_END) { ReadPtr++; break; }
                if(str[ReadPtr] != CHAR_QUOTE1) KLOG_ERROR(KSON_PARSE_OBJ_KEY_QUOTE,"");
                std::string key = ParseStr();
                if(ReadPtr >= str.size() || str[ReadPtr] != CHAR_SEPERATOR) KLOG_ERROR(KSON_PARSE_OBJ_SEPERATOR,"");
                ReadPtr++;
                Node val = ParseVal();
                bool IsFound = false;
                for(auto& [k,v] : obj)
                {
                    if(k == key) { v = std::move(val); IsFound = true; break; }
                }
                if(!IsFound) obj.emplace_back(key,std::move(val));
                if(ReadPtr >= str.size()) break;
                if(str[ReadPtr] == CHAR_COMMA) { ReadPtr++; continue; }
                else if(str[ReadPtr] == OBJ_END) { ReadPtr++; break; }
                else
                {
                    KLOG_ERROR(KSON_PARSE_OBJUE,"");
                    ReadPtr++;
                }
            }
            return Node(std::move(obj));
        }
        bool PeekIsImplicitObj() const
        {
            if (ReadPtr >= str.size() || str[ReadPtr] != CHAR_QUOTE1) return false;
            size_t i = ReadPtr + 1;
            bool isEscape = false;
            while (i < str.size())
            {
                char c = str[i];
                if (isEscape) { isEscape = false; i++; continue; }
                if (c == CHAR_ESCAPE1) { isEscape = true; i++; continue; }
                if (c == CHAR_QUOTE1) { i++; break; }
                i++;
            }
            return (i < str.size() && str[i] == CHAR_SEPERATOR);
        }
        Node ParseImplicitObj()
        {
            std::vector<std::pair<std::string,Node>> obj;
            while(ReadPtr < str.size())
            {
                if(str[ReadPtr] != CHAR_QUOTE1) KLOG_ERROR(KSON_PARSE_OBJ_KEY_QUOTE,"");
                std::string key = ParseStr();
                if(ReadPtr >= str.size() || str[ReadPtr] != CHAR_SEPERATOR) KLOG_ERROR(KSON_PARSE_OBJ_SEPERATOR,"");
                ReadPtr++;
                Node val = ParseVal();
                bool IsFound = false;
                for(auto& [k,v] : obj)
                {
                    if(k == key) { v = std::move(val); IsFound = true; break; }
                }
                if(!IsFound) obj.emplace_back(key,std::move(val));
                if(ReadPtr >= str.size()) break;
                if(str[ReadPtr] == CHAR_COMMA) { ReadPtr++; continue; }
                else { KLOG_ERROR(KSON_PARSE_OBJUE,""); ReadPtr++; }
            }
            return Node(std::move(obj));
        }
        public:
            explicit Parser(std::string_view str) : str(std::move(str)) {}
            Node Parse()
            {
                if (PeekIsImplicitObj())
                    return ParseImplicitObj();
                Node root = ParseVal();
                if(ReadPtr < str.size()) KLOG_WARNING(KSON_PARSE_TRAIL,"");
                return root;
            }
    };

    //--------------------------------NodePtr----------------------------------------------
    NodePtr::NodePtr() noexcept = default;
    NodePtr::NodePtr(std::shared_ptr<Node> r) noexcept : root_(std::move(r)) {}
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
    NodePtr NodePtr::Parse(std::string_view text) {
        auto root = std::make_shared<Node>(Parser(text).Parse());
        return NodePtr(root);
    }
    NodePtr NodePtr::ParseFile(std::string_view filepath) {
        std::string content = ReadFileRaw(filepath);
        return Parse(content);
    }
    std::string NodePtr::Str()  const { return std::string(Resolve()->AsStr()); }
    long long   NodePtr::Int()  const { return Resolve()->AsInt(); }
    double      NodePtr::Dec()  const { return Resolve()->AsDec(); }
    bool        NodePtr::Bool() const { return Resolve()->AsBool(); }
    std::size_t NodePtr::Size() const { return Resolve()->size(); }
    bool        NodePtr::Exists() const { return TryResolve() != nullptr; }

    static std::string NodeAutoString(const Node* n)
    {
        if (!n) return "null";
        switch (n->type())
        {
            case NodeType::kNull: return "null";
            case NodeType::kBool: return n->AsBool() ? "true" : "false";
            case NodeType::kInt:  return std::to_string(n->AsInt());
            case NodeType::kDec:
            {
                double d = n->AsDec();
                if (std::isinf(d)) return d < 0 ? "-inf" : "inf";
                if (std::isnan(d)) return "nan";
                std::ostringstream oss;
                oss << std::setprecision(15) << d;
                std::string s = oss.str();
                if (s.find('.') != std::string::npos)
                {
                    size_t last = s.find_last_not_of('0');
                    if (s[last] == '.') last--;
                    s.erase(last + 1);
                }
                return s;
            }
            case NodeType::kStr: return std::string(n->AsStr());
            case NodeType::kArr:
            {
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
        return NodeAutoString(TryResolve());
    }
    std::size_t NodePtr::size() const { return Size(); }

    std::vector<std::string> NodePtr::Vec() const
    {
        std::vector<std::string> out;
        const Node* n = TryResolve();
        if (!n) return out;
        if (n->IsArray())
        {
            for (const auto& e : n->AsArr())
                out.push_back(NodeAutoString(&e));
        }
        else if (n->IsObject())
        {
            for (const auto& [k, v] : n->AsObj())
                out.push_back(NodeAutoString(&v));
        }
        else
        {
            out.push_back(NodeAutoString(n));
        }
        return out;
    }

    //--------------------------------preprocess----------------------------------------------
    std::string Preprocess(std::string raw)
    {
        std::string res;
        res.resize(raw.size());
        size_t WriteIndex = 0, ReadIndex = 0;
        if(raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
           static_cast<unsigned char>(raw[1]) == 0xBB &&
           static_cast<unsigned char>(raw[2]) == 0xBF)
            ReadIndex = 3;
        enum State { Normal, InString, InEscape} state = Normal;
        for(;ReadIndex < raw.size();)
        {
            char c = raw[ReadIndex];
            switch (state)
            {
                case Normal:
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\b')
                        ReadIndex++;
                    else if(c == CHAR_COMMENT1)
                    {
                        auto end = raw.find('\n', ReadIndex);
                        ReadIndex = (end == std::string::npos) ? raw.size() : end;
                        ReadIndex++;
                    }
                    else if(c == CHAR_QUOTE1)
                    {
                        state = InString;
                        res[WriteIndex++] = c;
                        ReadIndex++;
                    }
                    else { res[WriteIndex++] = c; ReadIndex++; }
                    break;
                case InString:
                    if(c == CHAR_ESCAPE1)
                    {
                        state = InEscape;
                        res[WriteIndex++] = c;
                        ReadIndex++;
                    }
                    else if(c == CHAR_QUOTE1)
                    {
                        state = Normal;
                        res[WriteIndex++] = c;
                        ReadIndex++;
                    }
                    else { res[WriteIndex++] = c; ReadIndex++; }
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
    static void KSON_EnableVT100()
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        if (GetConsoleMode(h, &mode))
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    kson ReadKsonFile(std::string_view filename)
    {
        KSON_EnableVT100(); // 读取时开启控制台彩色（VT100）
        return read(Preprocess(ReadFileRaw(filename)));
    }

    //===============================================================
    //  文件读取（KFIO 并入 kson）
    //===============================================================
    namespace
    {
        std::string ToAbsolute(std::string_view path)
        {
            char buf[MAX_PATH] = {0};
            DWORD len = GetFullPathNameA(std::string(path).c_str(), MAX_PATH, buf, nullptr);
            if (len == 0 || len >= MAX_PATH)
                return std::string(path);
            return std::string(buf);
        }
        bool IsRelativePath(std::string_view path)
        {
            if (path.empty()) return true;
            if (path.size() >= 3 && path[1] == ':' && path[2] == '\\')
                return false;
            if (path[0] == '\\')
                return false;
            return true;
        }
        const std::string& GetProjectRoot()
        {
            static const std::string root = []() -> std::string
            {
                char buf[MAX_PATH];
                DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
                if (len == 0 || len >= MAX_PATH) return "";
                std::string dir(buf);
                size_t pos = dir.find_last_of('\\');
                if (pos == std::string::npos) return "";
                dir = dir.substr(0, pos);
                while (!dir.empty())
                {
                    std::string marker = dir + "\\CMakeLists.txt";
                    DWORD attr = GetFileAttributesA(marker.c_str());
                    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
                        return dir;
                    size_t sep = dir.find_last_of('\\');
                    if (sep == std::string::npos) break;
                    dir = dir.substr(0, sep);
                }
                return "";
            }();
            return root;
        }
    }

    std::string ReadFileRaw(std::string_view filepath)
    {
        std::string fullPath;
        if (IsRelativePath(filepath))
        {
            const std::string& root = GetProjectRoot();
            if (!root.empty())
                fullPath = root + "\\" + std::string(filepath);
            else
                fullPath = std::string(filepath);
        }
        else
        {
            fullPath = std::string(filepath);
        }

        std::ifstream File(fullPath, std::ios::binary);
        if (!File)
        {
            int err = errno;
            std::string absPath = ToAbsolute(filepath);
            std::string extra = absPath + " | errno=" + std::to_string(err)
                              + ": " + std::strerror(err);
            KLOG_FATAL(KSON_FILE_OPEN_FAIL, extra);
        }

        File.seekg(0, std::ios::end);
        std::size_t size = static_cast<std::size_t>(File.tellg());
        File.seekg(0, std::ios::beg);

        std::string content;
        content.resize(size);
        File.read(content.data(), static_cast<std::streamsize>(size));

        if (!File)
        {
            int err = errno;
            std::string extra = std::string(filepath) + " | errno=" + std::to_string(err)
                              + ": " + std::strerror(err);
            KLOG_FATAL(KSON_FILE_READ_FAIL, extra);
        }

        std::string filtered;
        filtered.reserve(content.size());
        for (std::size_t i = 0; i < content.size(); ++i)
        {
            if (content[i] != '\t')
                filtered += content[i];
        }
        return filtered;
    }

    std::vector<std::vector<MazeCell>> ReadMaze(std::string_view filepath, std::string_view maze_key)
    {
        std::string raw = ReadFileRaw(filepath);
        kson doc = read(Preprocess(raw));
        kson mazeNode = doc["maze"][maze_key];

        std::vector<std::vector<MazeCell>> grid;

        if (mazeNode.Resolve() && mazeNode.Resolve()->IsObject())
        {
            int w = (int)mazeNode["w"].Int();
            std::string rle = mazeNode["data"].Str();

            std::string decoded;
            decoded.reserve(rle.size() * 2);
            size_t i = 0;
            while (i < rle.size())
            {
                char c = rle[i++];
                int count = 0;
                while (i < rle.size() && isdigit((unsigned char)rle[i]))
                {
                    count = count * 10 + (rle[i] - '0');
                    i++;
                }
                decoded.append(count, c);
            }

            int rows = (int)decoded.size() / w;
            grid.reserve(rows);
            for (int r = 0; r < rows; r++)
            {
                std::vector<MazeCell> row;
                row.reserve(w);
                for (int c = 0; c < w; c++)
                {
                    char ch = decoded[r * w + c];
                    if      (ch == MAZE_WALL)  row.push_back(MazeCell::WALL);
                    else if (ch == MAZE_PATH)  row.push_back(MazeCell::PASSABLE);
                    else if (ch == MAZE_START) row.push_back(MazeCell::START);
                    else if (ch == MAZE_END)   row.push_back(MazeCell::END);
                    else                       row.push_back(MazeCell::PASSABLE);
                }
                grid.push_back(std::move(row));
            }
        }
        else
        {
            size_t rows = mazeNode.Size();
            grid.reserve(rows);
            for (size_t r = 0; r < rows; r++)
            {
                std::string rowStr = mazeNode[r][size_t(0)].Str();
                std::vector<MazeCell> row;
                row.reserve(rowStr.size());
                for (char c : rowStr)
                {
                    if      (c == MAZE_WALL)  row.push_back(MazeCell::WALL);
                    else if (c == MAZE_PATH)  row.push_back(MazeCell::PASSABLE);
                    else if (c == MAZE_START) row.push_back(MazeCell::START);
                    else if (c == MAZE_END)   row.push_back(MazeCell::END);
                    else                 row.push_back(MazeCell::PASSABLE);
                }
                grid.push_back(std::move(row));
            }
        }
        return grid;
    
    }
