#pragma once
#include "KFCommon.hpp"
#include "KLOGGER.hpp"
namespace KF
{
    namespace KSON
    {
        class NodePtr;
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
                // 构造
                Node() noexcept;
                explicit Node(bool val) noexcept;
                explicit Node(long long val) noexcept;
                explicit Node(double val) noexcept;
                explicit Node(std::string val) noexcept;
                explicit Node(std::vector<Node> val);
                explicit Node(std::vector<std::pair<std::string,Node>> val);

                // 类型
                NodeType type()  const noexcept;
                bool IsNull()    const noexcept;
                bool IsBool()    const noexcept;
                bool IsInt()     const noexcept;
                bool IsDec()     const noexcept;
                bool IsNumber()  const noexcept;  // int 或 dec
                bool IsString()  const noexcept;
                bool IsArray()   const noexcept;
                bool IsObject()  const noexcept;

                // 取值
                bool             AsBool()   const;
                long long        AsInt()    const;
                double           AsDec() const;
                std::string_view AsStr() const;
                const std::vector<Node>&     AsArr()  const;
                const std::vector<std::pair<std::string,Node>>&     AsObj() const;

                // 大小
                std::size_t size() const;
                
                // 查找（返回指针）
                const Node* find(std::string_view key) const; // 根据 键 查找对象中的键值对
                const Node* at(std::size_t index)      const; // 根据 下标 查找数组中的元素

                private:
                    // StorageType 节点存储类型 
                    /// @attention 与 NodeType 的区别是 : NodeType 是对外的

                    using arr_t = std::vector<Node>;
                    using obj_t = std::vector<std::pair<std::string,Node>>;
                    using storage_t = std::variant< 
                        std::monostate,
                        bool,std::string,double,long long,
                        arr_t,obj_t>;

                    storage_t Data;
        };
        struct PathSeg
        /// @attention 与 NodePtr 的区别是 : PathSeg 只记录一个位置片段，NodePtr 是一个完整的路径
        {
            std::string key;
            std::size_t index;
            PathSeg(std::string Key);
            PathSeg(std::size_t Index);
        };
        /// @brief KSON 树的指针，持有路径，提供访问方法
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
                std::size_t size() const;  // 小写别名，等价于 Size()，方便 arr.size() 风格
                bool Exists() const;

                /// @brief 自动分析值的类型，返回可打印的字符串表示
                /// @return 根据节点类型自动转换：
                ///         null→"null"  bool→"true"/"false"  int→数字串
                ///         dec→浮点串   str→字符串原文
                ///         arr→[e1, e2, ...]  obj→{"k": v, ...}
                /// @note  路径未找到时返回 "null"，不会 Fatal
                std::string Auto() const;
                
            private:
                std::shared_ptr<Node> root_;
                std::vector<PathSeg> path_;
                mutable const Node* cached_ = nullptr;
                
                const Node* ResolvePath(const std::vector<PathSeg>& path) const;
        };
        using kson = NodePtr;

        /////////////////////////////////////////////////////////

        ///@brief 读取文件并解析
        std::string Preprocess(std::string raw); // 预处理，将注释删除，将转义字符替换，去掉空格 换行等
        kson read(std::string_view processed);
        kson ReadKsonFile(std::string_view filename);
    }
}
