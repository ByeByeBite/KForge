// kbignum 伞形模块：re-export 四个子模块（int/dec/frc/cpx），
// 并在顶层补充跨类型自动提升运算符（需要四种类型均已可见）。
module;
#include <type_traits>

export module kbignum;

export import kbignum.kbigint;
export import kbignum.kbigdec;
export import kbignum.kbigfrc;
export import kbignum.kbigcpx;

export
{
    //===============================================================
    // 跨类型自动提升：以 MathRank 等级高者为结果类型
    //===============================================================
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator+(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} + b; else return a + A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator-(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} - b; else return a - A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator*(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} * b; else return a * A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator/(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} / b; else return a / A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    bool operator==(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} == b; else return a == A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    bool operator!=(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} != b; else return a != A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator<(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} < b; else return a < A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator<=(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} <= b; else return a <= A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator>(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} > b; else return a > A{b}; }
    template<class A, class B,
        std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
    auto operator>=(const A& a, const B& b)
    { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} >= b; else return a >= A{b}; }
}