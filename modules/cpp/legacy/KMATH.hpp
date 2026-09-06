#pragma once
#include "KFCommon.hpp"
#include "KLOGGER.hpp"
namespace KF
{
    namespace KMATH
    {
        /// @brief 大整数（前向声明，见后文完整定义）
        class BigInt; class BigDec;
        // 分数/复数模板：组件 N/D、R/I 可为四类数学类型（BigInt/BigDec/BigFrc/BigCpx）任意嵌套；
        // 默认参数保持 BigFrc=BigFrc<BigInt,BigInt>、BigCpx=BigCpx<BigDec,BigDec> 
        /// @brief 分数模板前向声明（默认 BigFrc<BigInt,BigInt>）
        template<class N = BigInt, class D = BigInt> class BigFrc;
        /// @brief 复数模板前向声明（默认 BigCpx<BigDec,BigDec>）
        template<class R = BigDec, class I = BigDec> class BigCpx;

        // ---- 类型等级（自动提升：BigInt < BigDec < BigFrc < BigCpx） ----
        /// @brief 数学类型等级元函数：非数学类型=0，四类依次=1..4，用于跨类型运算自动提示
        template<class T> struct MathRank { static constexpr int value = 0; };
        /// @brief MathRank 特化：BigInt 等级=1
        template<> struct MathRank<BigInt>{ static constexpr int value = 1; };
        /// @brief MathRank 特化：BigDec 等级=2
        template<> struct MathRank<BigDec>{ static constexpr int value = 2; };
        /// @brief MathRank 特化：BigFrc 等级=3
        template<class N, class D> struct MathRank<BigFrc<N,D>>{ static constexpr int value = 3; };
        /// @brief MathRank 特化：BigCpx 等级=4
        template<class R, class I> struct MathRank<BigCpx<R,I>>{ static constexpr int value = 4; };
        /// @brief 判断 T 是否为四类数学类型之一（等级 1..4）
        template<class T> struct IsMathType { static constexpr bool value = (MathRank<T>::value >= 1 && MathRank<T>::value <= 4); };

        /// @brief 无符号 limb 大小比较（base=1e9 小端，已裁剪高位零）
        inline int MagCmp(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            if(a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
            for(sdlimb i = static_cast<sdlimb>(a.size()) - 1; i >= 0; i--)
                if(a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
            return 0;
        }
        /// @brief limb 是否表示 0
        inline bool IsZero(const std::vector<limb>& v){ return v.size() == 1 && v[0] == 0; }

        /// @brief 提供判零/判负/零值/一值的统一接口
        template<class T, class Enable = void> struct BigTraits
        {
            // 默认（量级组件 BigInt/BigDec 及递归的 BigFrc/BigCpx）：判零判负按 limbs/isneg
            static bool is_zero(const T& v){ return IsZero(v.limbs); }
            static bool is_neg (const T& v){ return v.isneg; }
            static T zero(){ return T(); }
            static T one(){ return T(1); }
        };
        /// @brief int/double 等按数值判零判负
        template<class T> struct BigTraits<T, std::enable_if_t<std::is_arithmetic_v<T>>>
        {
            static bool is_zero(const T& v){ return v == 0; }
            static bool is_neg (const T& v){ return v < 0; }
            static T zero(){ return T(0); }
            static T one(){ return T(1); }
        };
        /// @brief 任意数学类型转 BigDec
        inline BigDec toBigDec(const BigDec& x);
        inline BigDec toBigDec(const BigInt& x);
        template<class N, class D> BigDec toBigDec(const BigFrc<N,D>& x);
        template<class R, class I> BigDec toBigDec(const BigCpx<R,I>& x);
        template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
        inline BigDec toBigDec(const T& x);
        /// @brief BigDec → 组件类型 R 的转换工具模板（BigFromDec<R>::from）
        template<class R, class Enable = void> struct BigFromDec;


        /// @brief  limbs 存储/状态(inf/nan)/量级算术/比较 转换
        class BigNum
        {
            public:
                std::vector<limb> limbs = {0}; // 无符号 base=1e9 小端
                bool isneg = false;            // 是否为负
                size_t scale = 0;              // 小数位数（BigInt 恒为 0）
                enum class State { Normal, Inf, NegInf, Nan };
                State state = State::Normal;

                bool IsInf()    const { return state == State::Inf  || state == State::NegInf; }
                bool IsNan()    const { return state == State::Nan; }
                bool IsNormal() const { return state == State::Normal; }
                std::string type() const;                    // scale==0 ? "int" : "dec"
                static BigNum ToBig(const std::string& str); // 十进制解析（含 scale）
                std::string ToStr() const;                   // 十进制字符串

                BigNum() = default;
                BigNum(const std::string& str);              // 十进制串（支持 inf/nan）
                explicit BigNum(State s);
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum(const T& num) { *this = FromArith(num); }

                BigNum operator+() const { return *this; }
                BigNum operator-() const;
                BigNum operator+(const BigNum& b) const;
                BigNum operator-(const BigNum& b) const;
                BigNum operator*(const BigNum& b) const;
                BigNum operator/(const BigNum& b) const; // 整除则整数，否则保留 9 位小数
                BigNum operator%(const BigNum& b) const;

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator+(const T& n) const { return *this + FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator-(const T& n) const { return *this - FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator*(const T& n) const { return *this * FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator/(const T& n) const { return *this / FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigNum operator%(const T& n) const { return *this % FromArith(n); }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator+(const T& a, const BigNum& b) { return b + a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator*(const T& a, const BigNum& b) { return b * a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator-(const T& a, const BigNum& b) { return BigNum(a) - b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator/(const T& a, const BigNum& b) { return BigNum(a) / b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigNum operator%(const T& a, const BigNum& b) { return BigNum(a) % b; }

                bool operator==(const BigNum& b) const;
                bool operator!=(const BigNum& b) const;
                bool operator< (const BigNum& b) const;
                bool operator<=(const BigNum& b) const;
                bool operator> (const BigNum& b) const;
                bool operator>=(const BigNum& b) const;

                friend std::ostream& operator<<(std::ostream& os, const BigNum& b){ os << b.ToStr(); return os; }
                friend std::istream& operator>>(std::istream& is, BigNum& b){ std::string t; if(is >> t) b = BigNum(t); return is; }

                explicit operator long long() const; // 范围内转换
                explicit operator double() const;
                bool IsInLongLongRange() const;
                bool IsInDoubleRange() const;

            protected:
                template<typename T> static BigNum FromArith(T v)
                {
                    if constexpr (std::is_integral_v<T>)
                        return BigNum(std::to_string(v));
                    else
                    {
                        std::ostringstream oss;
                        oss << std::setprecision(std::numeric_limits<T>::max_digits10) << v;
                        return BigNum(oss.str());
                    }
                }
        };

        /// @brief 大整数：scale 恒为 0 的纯整数，构造函数自动取整截断小数
        class BigInt : public BigNum
        {
            public:
                std::string type() const; // "int"（覆写基类）

                static BigInt ToBig(const std::string& str); // 整数串 → BigInt
                std::string ToStr() const;

                BigInt() = default;
                BigInt(const std::string& str); // 整数串（自动取整截断小数部分）
                explicit BigInt(State s);
                explicit BigInt(const BigNum& b);         // 十进制 → 取整
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt(const T& num) { *this = FromArith(num); }

                BigInt operator+() const { return *this; }
                BigInt operator-() const;
                BigInt operator+(const BigInt& b) const;
                BigInt operator-(const BigInt& b) const;
                BigInt operator*(const BigInt& b) const;
                BigInt operator/(const BigInt& b) const;   // 整除（截断向零）
                BigInt operator%(const BigInt& b) const;   // 余数符号随被除数

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt operator+(const T& n) const { return *this + FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt operator-(const T& n) const { return *this - FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt operator*(const T& n) const { return *this * FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt operator/(const T& n) const { return *this / FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigInt operator%(const T& n) const { return *this % FromArith(n); }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigInt operator+(const T& a, const BigInt& b) { return b + a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigInt operator*(const T& a, const BigInt& b) { return b * a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigInt operator-(const T& a, const BigInt& b) { return BigInt(a) - b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigInt operator/(const T& a, const BigInt& b) { return BigInt(a) / b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigInt operator%(const T& a, const BigInt& b) { return BigInt(a) % b; }

                bool operator==(const BigInt& b) const;
                bool operator!=(const BigInt& b) const;
                bool operator< (const BigInt& b) const;
                bool operator<=(const BigInt& b) const;
                bool operator> (const BigInt& b) const;
                bool operator>=(const BigInt& b) const;

                friend std::ostream& operator<<(std::ostream& os, const BigInt& b){ os << b.ToStr(); return os; }
                friend std::istream& operator>>(std::istream& is, BigInt& b){ std::string t; if(is >> t) b = BigInt(t); return is; }

                explicit operator long long() const; // 范围内 native 转换
                bool IsInLongLongRange() const;
                bool IsInDoubleRange() const;

            private:
                template<typename T> static BigInt FromArith(T num)
                {
                    if constexpr (std::is_floating_point_v<T>)
                        return BigInt(std::to_string(static_cast<long long>(num)));
                    else
                        return BigInt(std::to_string(num));
                }
        };
        /// @brief 大小数：整数 + scale 位小数，算术逻辑复用基类 BigNum，仅保持返回类型为 BigDec
        class BigDec : public BigNum
        {
            public:
                static BigDec ToBig(const std::string& str); // 十进制解析（含 scale）

                BigDec() = default;
                BigDec(const std::string& str) : BigNum(str) {}      // 十进制串（含 inf/nan）
                explicit BigDec(State s) : BigNum(s) {}
                explicit BigDec(const BigNum& bn) : BigNum(bn) {}    // 基类/十进制 → BigDec
                explicit BigDec(const BigInt& bi) : BigNum(bi) {}    // 大整数 → 小数（scale=0）
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec(const T& num) { *this = FromArith(num); }

                BigDec operator+() const { return *this; }
                BigDec operator-() const;                    // 算术主体复用基类 BigNum
                BigDec operator+(const BigDec& b) const { return BigDec(BigNum::operator+(b)); }
                BigDec operator-(const BigDec& b) const { return BigDec(BigNum::operator-(b)); }
                BigDec operator*(const BigDec& b) const { return BigDec(BigNum::operator*(b)); }
                BigDec operator/(const BigDec& b) const { return BigDec(BigNum::operator/(b)); }
                BigDec operator%(const BigDec& b) const { return BigDec(BigNum::operator%(b)); }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec operator+(const T& n) const { return *this + FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec operator-(const T& n) const { return *this - FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec operator*(const T& n) const { return *this * FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec operator/(const T& n) const { return *this / FromArith(n); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigDec operator%(const T& n) const { return *this % FromArith(n); }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigDec operator+(const T& a, const BigDec& b) { return b + a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigDec operator*(const T& a, const BigDec& b) { return b * a; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigDec operator-(const T& a, const BigDec& b) { return BigDec(a) - b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigDec operator/(const T& a, const BigDec& b) { return BigDec(a) / b; }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                friend BigDec operator%(const T& a, const BigDec& b) { return BigDec(a) % b; }
            private:
                template<typename T> static BigDec FromArith(T v)
                {
                    if constexpr (std::is_integral_v<T>)
                        return BigDec(std::to_string(v));
                    else
                    {
                        std::ostringstream oss;
                        oss << std::setprecision(std::numeric_limits<T>::max_digits10) << v;
                        return BigDec(oss.str());
                    }
                }
        };
        //===============================================================
        /// @brief BigFromDec 特化：BigDec → BigDec（恒等）
        template<> struct BigFromDec<BigDec>{ static BigDec from(const BigDec& d){ return d; } };
        /// @brief BigFromDec 特化：BigDec → BigInt（取整）
        template<> struct BigFromDec<BigInt>{ static BigInt from(const BigDec& d){ return BigInt(d); } };

        /// @brief 分数：分子 N/分母 D 组件可任意嵌套，分母恒归一化为正，默认 BigFrc<BigInt,BigInt>
        template<class N, class D>
        class BigFrc : public BigNum
        {
            public:
                N numerator;   // 分子（带符号，可为任意数学类型）
                D denominator; // 分母（恒为正，可为任意数学类型）
                std::string type() const { return "frac"; }

                BigFrc() : numerator(BigTraits<N>::zero()), denominator(BigTraits<D>::one()) {}
                BigFrc(const N& num, const D& den)
                {
                    if(BigTraits<D>::is_zero(den)) // 分母为 0 → 置 0
                    { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                    numerator = num; denominator = den;
                    if(BigTraits<D>::is_neg(den)) // 归一化：分母恒为正
                    { numerator = -numerator; denominator = -denominator; }
                }
                BigFrc(const BigDec& d) // 小数 → 精确分数（d = 数值 / 10^scale）
                {
                    if(d.IsNan() || d.IsInf() || d.state != BigDec::State::Normal)
                    { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                    if(IsZero(d.limbs))
                    { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                    BigDec intpart{ BigNum(d) }; intpart.scale = 0; // 去掉小数点，保留完整数字序列
                    numerator = BigFromDec<N>::from(intpart);
                    denominator = BigFromDec<D>::from(BigDec("1" + std::string(d.scale, '0')));
                }
                template<class T, std::enable_if_t<IsMathType<T>::value && !std::is_same_v<T,BigFrc<N,D>> && !std::is_same_v<T,BigDec>, int> = 0>
                BigFrc(const T& x) : numerator(BigFromDec<N>::from(toBigDec(x))), denominator(BigTraits<D>::one()) {}
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigFrc(const T& num) : BigFrc(BigDec(num)) {}

                BigFrc operator+() const { return *this; }
                BigFrc operator-() const { return BigFrc(-numerator, denominator); }
                BigFrc operator+(const BigFrc& b) const
                { return BigFrc(numerator*b.denominator + b.numerator*denominator, denominator*b.denominator); }
                BigFrc operator-(const BigFrc& b) const
                { return BigFrc(numerator*b.denominator - b.numerator*denominator, denominator*b.denominator); }
                BigFrc operator*(const BigFrc& b) const
                { return BigFrc(numerator*b.numerator, denominator*b.denominator); }
                BigFrc operator/(const BigFrc& b) const
                { return BigFrc(numerator*b.denominator, denominator*b.numerator); }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigFrc operator+(const T& n) const { return *this + BigFrc(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigFrc operator-(const T& n) const { return *this - BigFrc(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigFrc operator*(const T& n) const { return *this * BigFrc(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigFrc operator/(const T& n) const { return *this / BigFrc(BigDec(n)); }

                std::string ToStr() const  // "num/den"（den==1 时仅数字）
                {
                    if(BigTraits<D>::is_zero(denominator)) return "nan";
                    if(denominator == BigTraits<D>::one()) return toBigDec(numerator).ToStr();
                    return toBigDec(numerator).ToStr() + "/" + toBigDec(denominator).ToStr();
                }
                BigDec ToBigDec() const
                { return toBigDec(numerator) / toBigDec(denominator); } // 转小数（整除则整数，否则 9 位小数）
                bool operator==(const BigFrc& b) const
                { return numerator*b.denominator == b.numerator*denominator; }
                bool operator!=(const BigFrc& b) const { return !(*this == b); }
                bool operator< (const BigFrc& b) const
                { return numerator*b.denominator < b.numerator*denominator; }
                bool operator<=(const BigFrc& b) const { return (*this < b) || (*this == b); }
                bool operator> (const BigFrc& b) const { return !(*this <= b); }
                bool operator>=(const BigFrc& b) const { return !(*this < b); }

                friend std::ostream& operator<<(std::ostream& os, const BigFrc& f)
                { os << f.ToStr(); return os; }
                friend std::istream& operator>>(std::istream& is, BigFrc& f)
                { std::string t; if(is >> t) f = BigFrc<N,D>(BigDec(t)); return is; }
        };

        /// @brief n 次方根本身（前向声明，供 BigCpx::Abs 取模长）
        // Root 前向声明（供 Abs 取模长；默认 BigCpx = BigCpx<BigDec,BigDec>）
        BigCpx<> Root(const BigDec& a, const BigDec& n);

        /// @brief 复数：实部 R/虚部 I 组件可任意嵌套，提供共轭/模长及四则运算，默认 BigCpx<BigDec,BigDec>
        template<class R, class I>
        class BigCpx : public BigNum
        {
            public:
                R re;    // 实部（可为任意数学类型）
                I im;    // 虚部（可为任意数学类型）
                std::string type() const { return "cpx"; }

                BigCpx() : re(BigTraits<R>::zero()), im(BigTraits<I>::zero()) {}
                BigCpx(const R& r, const I& i) : re(r), im(i) {}
                explicit BigCpx(const BigDec& d) : re(BigFromDec<R>::from(d)), im(BigTraits<I>::zero()) {}
                template<class T, std::enable_if_t<IsMathType<T>::value && !std::is_same_v<T,BigCpx<R,I>> && !std::is_same_v<T,BigDec>, int> = 0>
                BigCpx(const T& x) : re(BigFromDec<R>::from(toBigDec(x))), im(BigTraits<I>::zero()) {}
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigCpx(const T& num) : re(BigFromDec<R>::from(BigDec(num))), im(BigTraits<I>::zero()) {}

                BigCpx Conj() const { return BigCpx(re, -im); }   // 共轭
                R Abs() const                                     // 模长 |z|
                {
                    const BigDec s = toBigDec(re)*toBigDec(re) + toBigDec(im)*toBigDec(im);
                    return BigFromDec<R>::from(Root(s, BigDec(2)).re);
                }
                std::string ToStr() const                        // "a+bi"
                {
                    if(BigTraits<I>::is_zero(im)) return toBigDec(re).ToStr();
                    const bool reZero = BigTraits<R>::is_zero(re);
                    const bool imNeg  = BigTraits<I>::is_neg(im);
                    std::string r = reZero ? "" : toBigDec(re).ToStr();
                    if(!imNeg) r += (r.empty() ? "" : "+") + toBigDec(im).ToStr() + "i";
                    else       r += toBigDec(im).ToStr() + "i";
                    return r;
                }

                BigCpx operator+() const { return *this; }
                BigCpx operator-() const { return BigCpx(-re, -im); }
                BigCpx operator+(const BigCpx& b) const
                { return BigCpx(re + b.re, im + b.im); }
                BigCpx operator-(const BigCpx& b) const
                { return BigCpx(re - b.re, im - b.im); }
                BigCpx operator*(const BigCpx& b) const
                { return BigCpx(BigFromDec<R>::from(toBigDec(re*b.re) - toBigDec(im*b.im)),
                                BigFromDec<I>::from(toBigDec(re*b.im) + toBigDec(im*b.re))); }
                BigCpx operator/(const BigCpx& b) const
                {
                    const BigDec den = toBigDec(b.re)*toBigDec(b.re) + toBigDec(b.im)*toBigDec(b.im);
                    if(IsZero(den.limbs)) return BigCpx(BigFromDec<R>::from(BigDec("nan")), BigFromDec<I>::from(BigDec("nan")));
                    return BigCpx(BigFromDec<R>::from((toBigDec(re)*toBigDec(b.re) + toBigDec(im)*toBigDec(b.im)) / den),
                                  BigFromDec<I>::from((toBigDec(im)*toBigDec(b.re) - toBigDec(re)*toBigDec(b.im)) / den));
                }

                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigCpx operator+(const T& n) const { return *this + BigCpx(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigCpx operator-(const T& n) const { return *this - BigCpx(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigCpx operator*(const T& n) const { return *this * BigCpx(BigDec(n)); }
                template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
                BigCpx operator/(const T& n) const { return *this / BigCpx(BigDec(n)); }

                bool operator==(const BigCpx& b) const { return re == b.re && im == b.im; }
                bool operator!=(const BigCpx& b) const { return !(*this == b); }

                friend std::ostream& operator<<(std::ostream& os, const BigCpx& z){ os << z.ToStr(); return os; }
                friend std::istream& operator>>(std::istream& is, BigCpx& z){ std::string t; if(is >> t) z = BigCpx(BigDec(t)); return is; }
        };

        //===============================================================
        //  组件通用：toBigDec / BigFromDec / BigTraits 的完整定义与偏特化
        //  （须放在 BigFrc/BigCpx 类完整定义之后才能使用完整类型）
        //===============================================================
        inline BigDec toBigDec(const BigDec& x){ return x; }
        inline BigDec toBigDec(const BigInt& x){ return BigDec(x); }
        template<class N, class D> BigDec toBigDec(const BigFrc<N,D>& x){ return x.ToBigDec(); }
        template<class R, class I> BigDec toBigDec(const BigCpx<R,I>& x){ return toBigDec(x.re); }
        /// @brief toBigDec 重载：标量组件（int/double 等）直接转为 BigDec
        template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
        inline BigDec toBigDec(const T& x){ return BigDec(x); }
        /// @brief ：BigDec → BigFrc（精确小数→分数）
        template<class N, class D> struct BigFromDec<BigFrc<N,D>>
        { static BigFrc<N,D> from(const BigDec& d){ return BigFrc<N,D>(d); } };
        /// @brief ：BigDec → BigCpx（作为实部）
        template<class R, class I> struct BigFromDec<BigCpx<R,I>>
        { static BigCpx<R,I> from(const BigDec& d){ return BigCpx<R,I>(d); } };
        /// @brief ：BigDec → 组件（数值截断）
        template<typename T> struct BigFromDec<T, std::enable_if_t<std::is_arithmetic_v<T>>>
        { static T from(const BigDec& d){ return static_cast<T>(std::stod(d.ToStr())); } };
        /// @brief BigFrc 以分子判零判负
        template<class N, class D> struct BigTraits<BigFrc<N,D>>
        {
            static bool is_zero(const BigFrc<N,D>& v){ return BigTraits<N>::is_zero(v.numerator); }
            static bool is_neg (const BigFrc<N,D>& v){ return BigTraits<N>::is_neg(v.numerator); }
            static BigFrc<N,D> zero(){ return BigFrc<N,D>(BigTraits<N>::zero(), BigTraits<D>::one()); }
            static BigFrc<N,D> one (){ return BigFrc<N,D>(BigTraits<N>::one(),  BigTraits<D>::one()); }
        };
        /// @brief BigTraits 特化：BigCpx 以实部判零判负
        template<class R, class I> struct BigTraits<BigCpx<R,I>>
        {
            static bool is_zero(const BigCpx<R,I>& v){ return BigTraits<R>::is_zero(v.re) && BigTraits<I>::is_zero(v.im); }
            static bool is_neg (const BigCpx<R,I>& v){ return BigTraits<R>::is_neg(v.re); }
            static BigCpx<R,I> zero(){ return BigCpx<R,I>(BigTraits<R>::zero(), BigTraits<I>::zero()); }
            static BigCpx<R,I> one (){ return BigCpx<R,I>(BigTraits<R>::one(),  BigTraits<I>::zero()); }
        };

        /// @brief 复数幂：z^n（整数指数精确重复平方法；小数指数走极坐标 double 近似）
        BigCpx<> Pow(const BigCpx<>& z, const BigDec& n);
        /// @brief 复数幂：整数指数会先转 BigDec 再调用（见 BigDec 重载）
        inline BigCpx<> Pow(const BigCpx<>& z, const BigInt& n){ return Pow(z, BigDec(n)); }
        /// @brief 复数共轭：实部不变、虚部取负
        inline BigCpx<> Conj(const BigCpx<>& z){ return z.Conj(); }
        /// @brief 复数模长 |z|（返回 BigDec）
        inline BigDec  Abs(const BigCpx<>& z){ return z.Abs(); } // 默认 BigCpx<BigDec,BigDec> 的 Abs() 即 BigDec 模长

        //===============================================================
        //  跨类型自动提升（BigInt < BigDec < BigFrc < BigCpx）
        //===============================================================
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator+(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} + b; else return a + A{b}; }
        /// @brief 跨类型加法：低等级类型自动提升到高等级后运算
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator-(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} - b; else return a - A{b}; }
        /// @brief 跨类型乘法：低等级类型自动提升到高等级后运算
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator*(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} * b; else return a * A{b}; }
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator/(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} / b; else return a / A{b}; }
        /// @brief 跨类型除法：低等级类型自动提升到高等级后运算
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        bool operator==(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} == b; else return a == A{b}; }
        /// @brief 跨类型不等比较：低等级类型自动提升到高等级后比较
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        bool operator!=(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} != b; else return a != A{b}; }
        /// @brief 跨类型小于比较：低等级类型自动提升到高等级后比较
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator<(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} < b; else return a < A{b}; }
        /// @brief 跨类型小于等于比较：低等级类型自动提升到高等级后比较
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator<=(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} <= b; else return a <= A{b}; }
        /// @brief 跨类型大于比较：低等级类型自动提升到高等级后比较
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator>(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} > b; else return a > A{b}; }
        /// @brief 跨类型大于等于比较：低等级类型自动提升到高等级后比较
        template<class A, class B,
            std::enable_if_t<IsMathType<A>::value && IsMathType<B>::value && !std::is_same_v<A,B>, int> = 0>
        auto operator>=(const A& a, const B& b)
        { if constexpr (MathRank<A>::value < MathRank<B>::value) return B{a} >= b; else return a >= A{b}; }

        
        //===============================================================
        //  自由函数
        //===============================================================
        /// @brief 数字串合法化：去除小数点及首尾导 0
        std::string Normalize(const std::string& str); // 合法化：去小数点 去前后导0
        /// @brief 对齐小数位（数值不变，要求 newScale >= x.scale）
        BigDec ScaleTo(const BigDec& x, size_t newScale); // 对齐小数位（数值不变，要求 newScale >= x.scale）
        /// @brief 非负整数最大公约数（欧几里得）
        BigInt BigGcd(BigInt a, BigInt b); // 非负整数最大公约数（欧几里得）
        /// @brief 幂：a^b（快速幂，支持负/分数指数与 inf-nan 规则）
        BigDec Pow(const BigDec& a, const BigDec& b); // 幂：a^b（快速幂，支持负/分数指数与 inf-nan 规则）
        /// @brief n 次方根：a^(1/n)；负数开偶次方返回虚数(实部0/虚部 BigDec)
        BigCpx<> Root(const BigDec& a, const BigDec& n); // n 次方根：a^(1/n)；负数开偶次方返回虚数(实部0/虚部 BigDec)
        /// @brief 随机整数（位数范围 + 符号：0随机/1全正/2全负）
        BigInt RandBigInt(std::pair<size_t,size_t> IntRand = {0,0}, int sign = 0); // 随机整数(位数范围 符号:0随机/1全正/2全负)
        /// @brief 随机小数（整数位数 + 小数位数 + 符号）
        BigDec RandBigDec(std::pair<size_t,size_t> IntRand = {0,0}, std::pair<size_t,size_t> DecRand = {0,0}, int sign = 0); // 随机小数(整数位数 小数位数 符号)
        /// @brief 随机小数别名，等价于 RandBigDec
        inline BigDec RandBigNum(std::pair<size_t,size_t> i = {0,0}, std::pair<size_t,size_t> d = {0,0}, int s = 0){ return RandBigDec(i, d, s); }

    }
}
