module;
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cctype>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include <cmath>
#include <cstdlib>
#include <random>
#include <ostream>
#include <istream>

// 日志宏：自动捕获调用位置（本模块内使用，位于  内）
#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module kbignum;

import klogger;

export
{
    using limb = uint32_t;   // 基础分块
    using dlimb = uint64_t;  // double limb
    using slimb = int32_t;   // signed limb
    using sdlimb = int64_t;  // signed double limb
    //===============================================================
    //  大整数
    //===============================================================
    class BigInt; class BigDec;
    template<class N = BigInt, class D = BigInt> class BigFrc;
    template<class R = BigDec, class I = BigDec> class BigCpx;

    /// @brief 数学类型等级元函数：非数学类型=0，四类依次=1..4
    template<class T> struct MathRank { static constexpr int value = 0; };
    template<> struct MathRank<BigInt>{ static constexpr int value = 1; };
    template<> struct MathRank<BigDec>{ static constexpr int value = 2; };
    template<class N, class D> struct MathRank<BigFrc<N,D>>{ static constexpr int value = 3; };
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
    inline bool IsZero(const std::vector<limb>& v){ return v.size() == 1 && v[0] == 0; }

    template<class T, class Enable = void> struct BigTraits
    {
        static bool is_zero(const T& v){ return IsZero(v.limbs); }
        static bool is_neg (const T& v){ return v.isneg; }
        static T zero(){ return T(); }
        static T one(){ return T(1); }
    };
    template<class T> struct BigTraits<T, std::enable_if_t<std::is_arithmetic_v<T>>>
    {
        static bool is_zero(const T& v){ return v == 0; }
        static bool is_neg (const T& v){ return v < 0; }
        static T zero(){ return T(0); }
        static T one(){ return T(1); }
    };
    inline BigDec toBigDec(const BigDec& x);
    inline BigDec toBigDec(const BigInt& x);
    template<class N, class D> BigDec toBigDec(const BigFrc<N,D>& x);
    template<class R, class I> BigDec toBigDec(const BigCpx<R,I>& x);
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
    inline BigDec toBigDec(const T& x);
    template<class R, class Enable = void> struct BigFromDec;

    class BigNum
    {
        public:
            std::vector<limb> limbs = {0};
            bool isneg = false;
            size_t scale = 0;
            enum class State { Normal, Inf, NegInf, Nan };
            State state = State::Normal;

            bool IsInf()    const { return state == State::Inf  || state == State::NegInf; }
            bool IsNan()    const { return state == State::Nan; }
            bool IsNormal() const { return state == State::Normal; }
            std::string type() const;
            static BigNum ToBig(const std::string& str);
            std::string ToStr() const;

            BigNum() = default;
            BigNum(const std::string& str);
            explicit BigNum(State s);
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigNum(const T& num) { *this = FromArith(num); }

            BigNum operator+() const { return *this; }
            BigNum operator-() const;
            BigNum operator+(const BigNum& b) const;
            BigNum operator-(const BigNum& b) const;
            BigNum operator*(const BigNum& b) const;
            BigNum operator/(const BigNum& b) const;
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

            explicit operator long long() const;
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

    class BigInt : public BigNum
    {
        public:
            std::string type() const;
            static BigInt ToBig(const std::string& str);
            std::string ToStr() const;

            BigInt() = default;
            BigInt(const std::string& str);
            explicit BigInt(State s);
            explicit BigInt(const BigNum& b);
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigInt(const T& num) { *this = FromArith(num); }

            BigInt operator+() const { return *this; }
            BigInt operator-() const;
            BigInt operator+(const BigInt& b) const;
            BigInt operator-(const BigInt& b) const;
            BigInt operator*(const BigInt& b) const;
            BigInt operator/(const BigInt& b) const;
            BigInt operator%(const BigInt& b) const;

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

            explicit operator long long() const;
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

    class BigDec : public BigNum
    {
        public:
            static BigDec ToBig(const std::string& str);

            BigDec() = default;
            BigDec(const std::string& str) : BigNum(str) {}
            explicit BigDec(State s) : BigNum(s) {}
            explicit BigDec(const BigNum& bn) : BigNum(bn) {}
            explicit BigDec(const BigInt& bi) : BigNum(bi) {}
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec(const T& num) { *this = FromArith(num); }

            BigDec operator+() const { return *this; }
            BigDec operator-() const;
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

    template<> struct BigFromDec<BigDec>{ static BigDec from(const BigDec& d){ return d; } };
    template<> struct BigFromDec<BigInt>{ static BigInt from(const BigDec& d){ return BigInt(d); } };

    /// @brief 分数：分子 N/分母 D 组件可任意嵌套
    template<class N, class D>
    class BigFrc : public BigNum
    {
        public:
            N numerator;
            D denominator;
            std::string type() const { return "frac"; }

            BigFrc() : numerator(BigTraits<N>::zero()), denominator(BigTraits<D>::one()) {}
            BigFrc(const N& num, const D& den)
            {
                if(BigTraits<D>::is_zero(den))
                { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                numerator = num; denominator = den;
                if(BigTraits<D>::is_neg(den))
                { numerator = -numerator; denominator = -denominator; }
            }
            BigFrc(const BigDec& d)
            {
                if(d.IsNan() || d.IsInf() || d.state != BigDec::State::Normal)
                { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                if(IsZero(d.limbs))
                { numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                BigDec intpart{ BigNum(d) }; intpart.scale = 0;
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

            std::string ToStr() const
            {
                if(BigTraits<D>::is_zero(denominator)) return "nan";
                if(denominator == BigTraits<D>::one()) return toBigDec(numerator).ToStr();
                return toBigDec(numerator).ToStr() + "/" + toBigDec(denominator).ToStr();
            }
            BigDec ToBigDec() const
            { return toBigDec(numerator) / toBigDec(denominator); }
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

    BigCpx<> Root(const BigDec& a, const BigDec& n);

    /// @brief 复数：实部 R/虚部 I 组件可任意嵌套
    template<class R, class I>
    class BigCpx : public BigNum
    {
        public:
            R re;
            I im;
            std::string type() const { return "cpx"; }

            BigCpx() : re(BigTraits<R>::zero()), im(BigTraits<I>::zero()) {}
            BigCpx(const R& r, const I& i) : re(r), im(i) {}
            explicit BigCpx(const BigDec& d) : re(BigFromDec<R>::from(d)), im(BigTraits<I>::zero()) {}
            template<class T, std::enable_if_t<IsMathType<T>::value && !std::is_same_v<T,BigCpx<R,I>> && !std::is_same_v<T,BigDec>, int> = 0>
            BigCpx(const T& x) : re(BigFromDec<R>::from(toBigDec(x))), im(BigTraits<I>::zero()) {}
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigCpx(const T& num) : re(BigFromDec<R>::from(BigDec(num))), im(BigTraits<I>::zero()) {}

            BigCpx Conj() const { return BigCpx(re, -im); }
            R Abs() const
            {
                const BigDec s = toBigDec(re)*toBigDec(re) + toBigDec(im)*toBigDec(im);
                return BigFromDec<R>::from(Root(s, BigDec(2)).re);
            }
            std::string ToStr() const
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

    inline BigDec toBigDec(const BigDec& x){ return x; }
    inline BigDec toBigDec(const BigInt& x){ return BigDec(x); }
    template<class N, class D> BigDec toBigDec(const BigFrc<N,D>& x){ return x.ToBigDec(); }
    template<class R, class I> BigDec toBigDec(const BigCpx<R,I>& x){ return toBigDec(x.re); }
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    inline BigDec toBigDec(const T& x){ return BigDec(x); }
    template<class N, class D> struct BigFromDec<BigFrc<N,D>>
    { static BigFrc<N,D> from(const BigDec& d){ return BigFrc<N,D>(d); } };
    template<class R, class I> struct BigFromDec<BigCpx<R,I>>
    { static BigCpx<R,I> from(const BigDec& d){ return BigCpx<R,I>(d); } };
    template<typename T> struct BigFromDec<T, std::enable_if_t<std::is_arithmetic_v<T>>>
    { static T from(const BigDec& d){ return static_cast<T>(std::stod(d.ToStr())); } };
    template<class N, class D> struct BigTraits<BigFrc<N,D>>
    {
        static bool is_zero(const BigFrc<N,D>& v){ return BigTraits<N>::is_zero(v.numerator); }
        static bool is_neg (const BigFrc<N,D>& v){ return BigTraits<N>::is_neg(v.numerator); }
        static BigFrc<N,D> zero(){ return BigFrc<N,D>(BigTraits<N>::zero(), BigTraits<D>::one()); }
        static BigFrc<N,D> one (){ return BigFrc<N,D>(BigTraits<N>::one(),  BigTraits<D>::one()); }
    };
    template<class R, class I> struct BigTraits<BigCpx<R,I>>
    {
        static bool is_zero(const BigCpx<R,I>& v){ return BigTraits<R>::is_zero(v.re) && BigTraits<I>::is_zero(v.im); }
        static bool is_neg (const BigCpx<R,I>& v){ return BigTraits<R>::is_neg(v.re); }
        static BigCpx<R,I> zero(){ return BigCpx<R,I>(BigTraits<R>::zero(), BigTraits<I>::zero()); }
        static BigCpx<R,I> one (){ return BigCpx<R,I>(BigTraits<R>::one(),  BigTraits<I>::zero()); }
    };

    /// @brief 复数幂：z^n
    BigCpx<> Pow(const BigCpx<>& z, const BigDec& n);
    inline BigCpx<> Pow(const BigCpx<>& z, const BigInt& n){ return Pow(z, BigDec(n)); }
    inline BigCpx<> Conj(const BigCpx<>& z){ return z.Conj(); }
    inline BigDec  Abs(const BigCpx<>& z){ return z.Abs(); }

    // ---- 跨类型自动提升 ----
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

    // ---- 自由函数 ----
    std::string Normalize(const std::string& str);
    BigDec ScaleTo(const BigDec& x, size_t newScale);
    BigInt BigGcd(BigInt a, BigInt b);
    BigDec Pow(const BigDec& a, const BigDec& b);
    BigCpx<> Root(const BigDec& a, const BigDec& n);
    BigInt RandBigInt(std::pair<size_t,size_t> IntRand = {0,0}, int sign = 0);
    BigDec RandBigDec(std::pair<size_t,size_t> IntRand = {0,0}, std::pair<size_t,size_t> DecRand = {0,0}, int sign = 0);
    inline BigDec RandBigNum(std::pair<size_t,size_t> i = {0,0}, std::pair<size_t,size_t> d = {0,0}, int s = 0){ return RandBigDec(i, d, s); }
}



    //===============================================================
    //  内部 limb 原语（base=1e9，小端，裁剪高位零）
    //===============================================================
    namespace
    {
        constexpr char   CHAR_NEG = '-';
        constexpr char   CHAR_POS = '+';
        constexpr char   CHAR_DOT = '.';
        constexpr size_t KEEP = 9;
        constexpr dlimb  BASE = 1000000000ULL;
        constexpr size_t BASEEXP = 9;

        inline void trim(std::vector<limb>& v){ while(v.size() > 1 && v.back() == 0) v.pop_back(); }

        // 前向声明：bn_Pow10 在 ScaleLimb 之后定义，需先声明才能在 ScaleLimb 内调用
        sdlimb bn_Pow10(sdlimb n);

        std::vector<limb> add(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            const size_t n = (std::max)(a.size(), b.size());
            std::vector<limb> r(n + 1, 0);
            dlimb carry = 0;
            for(size_t i = 0; i < n + 1; i++)
            {
                dlimb s = (i < a.size() ? a[i] : 0) + (i < b.size() ? b[i] : 0) + carry;
                r[i] = static_cast<limb>(s % BASE);
                carry = s / BASE;
            }
            trim(r);
            return r;
        }
        std::vector<limb> sub(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            std::vector<limb> r(a.size(), 0);
            slimb borrow = 0;
            for(size_t i = 0; i < a.size(); i++)
            {
                slimb d = static_cast<slimb>(a[i]) - (i < b.size() ? static_cast<slimb>(b[i]) : 0) - borrow;
                if(d < 0) { r[i] = static_cast<limb>(d + BASE); borrow = 1; }
                else      { r[i] = static_cast<limb>(d);        borrow = 0; }
            }
            trim(r);
            return r;
        }
        std::vector<limb> mul(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            std::vector<limb> r(a.size() + b.size() + 3, 0);
            for(size_t i = 0; i < a.size(); i++)
            {
                dlimb carry = 0;
                const limb aval = a[i];
                for(size_t j = 0; j < b.size(); j++)
                {
                    const dlimb t = (dlimb)aval * b[j] + r[i + j] + carry;
                    r[i + j] = static_cast<limb>(t % BASE);
                    carry    = t / BASE;
                }
                size_t k = i + b.size();
                while(carry)
                {
                    const dlimb t = (dlimb)r[k] + carry;
                    r[k] = static_cast<limb>(t % BASE);
                    carry = t / BASE;
                    ++k;
                }
            }
            trim(r);
            return r;
        }
        std::vector<limb> ScaleLimb(const std::vector<limb>& v, size_t k)
        {
            if(k == 0) return v;
            std::vector<limb> r = v;
            const size_t hi = k / BASEEXP, lo = k % BASEEXP;
            if(lo)
            {
                const dlimb mult = bn_Pow10(lo);   // 局部助手（kutil 已并入 kcli，此处自含）
                dlimb carry = 0;
                for(size_t i = 0; i < r.size(); i++)
                {
                    const dlimb t = (dlimb)r[i] * mult + carry;
                    r[i] = static_cast<limb>(t % BASE);
                    carry = t / BASE;
                }
                if(carry) r.push_back(static_cast<limb>(carry));
            }
            if(hi) r.insert(r.begin(), hi, 0);
            trim(r);
            return r;
        }
        void div(const std::vector<limb>& a, const std::vector<limb>& b,
                 std::vector<limb>& quot, std::vector<limb>& rem)
        {
            if(MagCmp(a, b) < 0) { quot = {0}; rem = a; return; }
            const size_t n = a.size(), m = b.size();
            const size_t qlen = n - m + 1;
            quot.assign(qlen, 0);
            rem.assign(1, 0);
            for(sdlimb i = static_cast<sdlimb>(n) - 1; i >= 0; i--)
            {
                rem.insert(rem.begin(), 0);
                rem[0] = a[i];
                trim(rem);
                slimb lo = 0, hi = static_cast<slimb>(BASE);
                while(lo + 1 < hi)
                {
                    const slimb mid = lo + (hi - lo) / 2;
                    std::vector<limb> t = mul(b, std::vector<limb>{static_cast<limb>(mid)});
                    trim(t);
                    if(MagCmp(t, rem) <= 0) lo = mid;
                    else                    hi = mid;
                }
                if(i < static_cast<sdlimb>(qlen)) quot[i] = static_cast<limb>(lo);
                if(lo > 0)
                {
                    std::vector<limb> t = mul(b, std::vector<limb>{static_cast<limb>(lo)});
                    trim(t);
                    rem = sub(rem, t);
                }
            }
            trim(quot);
            trim(rem);
        }
        bool toLL(const std::vector<limb>& v, bool neg, long long& out)
        {
            if(v.size() > 2) return false;
            unsigned long long mag = v[0];
            if(v.size() == 2) mag += (unsigned long long)v[1] * BASE;
            if(mag > 9223372036854775807ULL) return false;
            out = neg ? -static_cast<long long>(mag) : static_cast<long long>(mag);
            return true;
        }
        size_t sigDigits(const std::string& s)
        {
            size_t n = 0;
            for(char c : s)
                if(c >= '0' && c <= '9') n++;
            return n;
        }
        // 局部助手：kbignum 独立，不依赖 kcli 的 RandInt/Pow10
        std::random_device bn_rd;
        std::mt19937 bn_gen(bn_rd());
        sdlimb bn_RandInt(sdlimb min, sdlimb max)
        {
            std::uniform_int_distribution<sdlimb> dist(min, max);
            return dist(bn_gen);
        }
        sdlimb bn_Pow10(sdlimb n)
        {
            sdlimb res = 1;
            for (sdlimb i = 0; i < n; i++) res *= 10;
            return res;
        }
    }

    // 局部 Pow10 供内部 ScaleLimb 使用（放在命名空间内可被上面的匿名命名空间访问）
    static sdlimb Pow10Local(sdlimb n) { sdlimb r = 1; for (sdlimb i = 0; i < n; i++) r *= 10; return r; }

    static int CmpParts(const std::vector<limb>& la, bool na,
                        const std::vector<limb>& lb, bool nb, size_t sa, size_t sb)
    {
        if(na != nb)
        {
            if(IsZero(la) && IsZero(lb)) return 0;
            return na ? -1 : 1;
        }
        const size_t s = (std::max)(sa, sb);
        const int c = (sa == sb) ? MagCmp(la, lb) : MagCmp(ScaleLimb(la, s - sa), ScaleLimb(lb, s - sb));
        return na ? -c : c;
    }
    static int CmpDec(const BigDec& a, const BigDec& b)
    { return CmpParts(a.limbs, a.isneg, b.limbs, b.isneg, a.scale, b.scale); }
    static int CmpInt(const BigInt& a, const BigInt& b)
    { return CmpParts(a.limbs, a.isneg, b.limbs, b.isneg, 0, 0); }
    static int CmpNum(const BigNum& a, const BigNum& b)
    { return CmpParts(a.limbs, a.isneg, b.limbs, b.isneg, a.scale, b.scale); }

    std::string Normalize(const std::string& str)
    {
        size_t start = str.find_first_of("-+.0123456789");
        if(start == std::string::npos) return "0";
        size_t end = str.find_last_of(".0123456789");
        if(end == std::string::npos) return "0";
        std::string res;
        res.resize(end - start + 2);
        bool dot = false, isneg = false, numstart = false;
        size_t wp = 1, rp = start;
        while(rp <= end)
        {
            const char c = str[rp];
            if(isdigit(static_cast<unsigned char>(c)))
            {
                if(c == '0' && !numstart && !dot) { rp++; continue; }
                numstart = true;
                res[wp++] = c; rp++;
            }
            else if(c == CHAR_DOT && !dot)
            {
                if(numstart == 0) { res.insert(res.begin() + wp, 1, '0'); wp++; }
                dot = true;
                res[wp++] = CHAR_DOT; rp++;
            }
            else if(!numstart && (c == CHAR_NEG || c == CHAR_POS))
            {
                if(c == CHAR_NEG) isneg = !isneg;
                rp++;
            }
            else rp++;
        }
        if(!numstart) return "0";
        res[0] = isneg ? CHAR_NEG : CHAR_POS;
        res.resize(wp);
        if(!res.empty() && res.back() == CHAR_DOT) res.pop_back();
        if(dot)
        {
            while(!res.empty() && res.back() == '0') res.pop_back();
            if(!res.empty() && res.back() == CHAR_DOT) res.pop_back();
        }
        if(res.size() <= 1) return "0";
        for(size_t i = 1; i < res.size(); i++)
            if(res[i] != '0') return res;
        return "0";
    }

    BigInt BigInt::ToBig(const std::string& str)
    {
        BigInt res;
        res.limbs.clear();
        res.isneg = (str[0] == CHAR_NEG);
        res.state = State::Normal;
        std::string digits;
        for(size_t i = 1; i < str.size(); i++)
        {
            if(str[i] == CHAR_DOT) break;
            if(isdigit(static_cast<unsigned char>(str[i]))) digits += str[i];
        }
        if(digits.empty()) { res.limbs = {0}; res.isneg = false; return res; }
        dlimb pos = static_cast<dlimb>(digits.size());
        while(pos > 0)
        {
            dlimb s = (pos >= BASEEXP) ? (pos - BASEEXP) : 0;
            res.limbs.push_back(static_cast<limb>(
                std::stoll(digits.substr(static_cast<size_t>(s), static_cast<size_t>(pos - s)))));
            pos = s;
        }
        trim(res.limbs);
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    BigInt::BigInt(const std::string& str)
    {
        std::string low = str; size_t b = 0, e = low.size();
        while(b < e && isspace(static_cast<unsigned char>(low[b]))) b++;
        while(e > b && isspace(static_cast<unsigned char>(low[e-1]))) e--;
        for(size_t i = b; i < e; i++) low[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(low[i])));
        const std::string tok = low.substr(b, e - b);
        if(tok == "inf" || tok == "+inf") { state = State::Inf;    return; }
        if(tok == "-inf")                 { state = State::NegInf; return; }
        if(tok == "nan")                  { state = State::Nan;    return; }
        *this = ToBig(Normalize(str));
    }
    BigInt::BigInt(State s)
    {
        state = s;
        limbs = {0};
        isneg = (s == State::NegInf);
    }
    BigInt::BigInt(const BigNum& b)
    { *this = BigInt(b.ToStr()); }
    std::string BigInt::type() const
    {
        switch(state)
        {
            case State::Nan: return "nan";
            case State::Inf: return "inf";
            case State::NegInf: return "-inf";
            case State::Normal: break;
        }
        return "int";
    }
    std::string BigInt::ToStr() const
    {
        if(state == State::Nan) return "nan";
        if(state == State::Inf) return "inf";
        if(state == State::NegInf) return "-inf";
        if(IsZero(limbs)) return "0";
        std::string digits = std::to_string(limbs.back());
        for(sdlimb i = static_cast<sdlimb>(limbs.size()) - 2; i >= 0; i--)
        {
            std::string ch = std::to_string(limbs[i]);
            ch = std::string(BASEEXP - ch.size(), '0') + ch;
            digits += ch;
        }
        return isneg ? "-" + digits : digits;
    }
    BigInt BigInt::operator-() const
    {
        BigInt r = *this;
        if(state == State::Inf) r.state = State::NegInf;
        else if(state == State::NegInf) r.state = State::Inf;
        else if(state == State::Normal && !IsZero(limbs)) r.isneg = !isneg;
        return r;
    }
    BigInt BigInt::operator+(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state != b.state) return BigInt(State::Nan);
            return BigInt(state != State::Normal ? state : b.state);
        }
        long long x, y;
        if(toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y)) return BigInt(x + y);
        BigInt res; res.state = State::Normal;
        if(isneg == b.isneg)
        {
            res.limbs = add(limbs, b.limbs);
            res.isneg = isneg;
        }
        else
        {
            const int c = MagCmp(limbs, b.limbs);
            if(c == 0) return BigInt(0);
            if(c > 0) { res.limbs = sub(limbs, b.limbs); res.isneg = isneg; }
            else      { res.limbs = sub(b.limbs, limbs); res.isneg = b.isneg; }
        }
        return res;
    }
    BigInt BigInt::operator-(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state == b.state) return BigInt(State::Nan);
            if(state != State::Normal) return BigInt(state);
            return BigInt(b.state == State::Inf ? State::NegInf : State::Inf);
        }
        long long x, y;
        if(toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y)) return BigInt(x - y);
        BigInt res; res.state = State::Normal;
        if(isneg != b.isneg)
        {
            res.limbs = add(limbs, b.limbs);
            res.isneg = isneg;
        }
        else
        {
            const int c = MagCmp(limbs, b.limbs);
            if(c == 0) return BigInt(0);
            if(c > 0) { res.limbs = sub(limbs, b.limbs); res.isneg = isneg; }
            else      { res.limbs = sub(b.limbs, limbs); res.isneg = !isneg; }
        }
        return res;
    }
    BigInt BigInt::operator*(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        const bool ai = IsInf(), bi = b.IsInf();
        if(ai || bi)
        {
            const bool az = state == State::Normal && IsZero(limbs);
            const bool bz = b.state == State::Normal && IsZero(b.limbs);
            if((ai && bz) || (bi && az)) return BigInt(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            return BigInt((an ^ bn) ? State::NegInf : State::Inf);
        }
        BigInt res; res.state = State::Normal;
        res.limbs = mul(limbs, b.limbs);
        res.isneg = isneg ^ b.isneg;
        return res;
    }
    BigInt BigInt::operator/(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(IsInf() || b.IsInf())
        {
            if(IsInf() && b.IsInf()) return BigInt(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            if(IsInf()) return BigInt((an ^ bn) ? State::NegInf : State::Inf);
            return BigInt(0);
        }
        if(IsZero(b.limbs))
        {
            KLOG_ERROR(KBIGNUM_DIVBYZERO, "divide by zero");
            return IsZero(limbs) ? BigInt(State::Nan)
                                 : BigInt((isneg ^ b.isneg) ? State::NegInf : State::Inf);
        }
        if(IsZero(limbs)) return BigInt(0);
        std::vector<limb> q, r;
        div(limbs, b.limbs, q, r);
        BigInt res; res.state = State::Normal;
        res.limbs = q;
        res.isneg = isneg ^ b.isneg;
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    BigInt BigInt::operator%(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(IsInf() || b.IsInf()) return BigInt(State::Nan);
        if(IsZero(b.limbs)) { KLOG_ERROR(KBIGNUM_DIVBYZERO, "module by zero"); return BigInt(State::Nan); }
        if(IsZero(limbs)) return BigInt(0);
        std::vector<limb> q, r;
        div(limbs, b.limbs, q, r);
        BigInt res; res.state = State::Normal;
        res.limbs = r;
        res.isneg = isneg;
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    bool BigInt::operator==(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state != State::Normal || b.state != State::Normal) return state == b.state;
        return CmpInt(*this, b) == 0;
    }
    bool BigInt::operator!=(const BigInt& b) const { return !(*this == b); }
    bool BigInt::operator<(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state == State::NegInf) return b.state != State::NegInf;
        if(b.state == State::Inf) return state != State::Inf;
        if(state == State::Inf) return false;
        if(b.state == State::NegInf) return false;
        return CmpInt(*this, b) < 0;
    }
    bool BigInt::operator<=(const BigInt& b) const { return (*this < b) || (*this == b); }
    bool BigInt::operator>(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this <= b);
    }
    bool BigInt::operator>=(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this < b);
    }
    BigInt::operator long long() const { return std::stoll(ToStr()); }
    bool BigInt::IsInLongLongRange() const
    {
        if(state != State::Normal || limbs.size() > 2) return false;
        unsigned long long mag = limbs[0];
        if(limbs.size() == 2) mag += (unsigned long long)limbs[1] * BASE;
        return mag <= 9223372036854775807ULL;
    }
    bool BigInt::IsInDoubleRange() const { return sigDigits(ToStr()) <= 308; }

    BigNum::BigNum(const std::string& str)
    {
        std::string low = str; size_t b = 0, e = low.size();
        while(b < e && isspace(static_cast<unsigned char>(low[b]))) b++;
        while(e > b && isspace(static_cast<unsigned char>(low[e-1]))) e--;
        for(size_t i = b; i < e; i++) low[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(low[i])));
        const std::string tok = low.substr(b, e - b);
        if(tok == "inf" || tok == "+inf") { state = State::Inf;    return; }
        if(tok == "-inf")                 { state = State::NegInf; return; }
        if(tok == "nan")                  { state = State::Nan;    return; }
        *this = ToBig(Normalize(str));
    }
    BigNum::BigNum(State s)
    {
        state = s; limbs = {0};
        isneg = (s == State::NegInf);
        scale = 0;
    }
    std::string BigNum::type() const
    {
        switch(state)
        {
            case State::Nan: return "nan";
            case State::Inf: return "inf";
            case State::NegInf: return "-inf";
            case State::Normal: break;
        }
        return (scale == 0) ? "int" : "dec";
    }
    BigNum BigNum::ToBig(const std::string& str)
    {
        BigNum res;
        res.limbs.clear();
        res.state = State::Normal;
        res.isneg = (str[0] == CHAR_NEG);
        size_t dot = str.find(CHAR_DOT);
        res.scale = (dot == std::string::npos) ? 0 : (str.size() - dot - 1);
        std::string digits;
        digits.reserve(str.size());
        for(size_t i = 1; i < str.size(); i++)
            if(str[i] != CHAR_DOT) digits += str[i];
        if(digits.empty()) { res.limbs = {0}; res.isneg = false; return res; }
        dlimb pos = static_cast<dlimb>(digits.size());
        while(pos > 0)
        {
            dlimb s = (pos >= BASEEXP) ? (pos - BASEEXP) : 0;
            res.limbs.push_back(static_cast<limb>(
                std::stoll(digits.substr(static_cast<size_t>(s), static_cast<size_t>(pos - s)))));
            pos = s;
        }
        trim(res.limbs);
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    std::string BigNum::ToStr() const
    {
        if(state == State::Nan) return "nan";
        if(state == State::Inf) return "inf";
        if(state == State::NegInf) return "-inf";
        if(IsZero(limbs)) return "0";
        std::string digits = std::to_string(limbs.back());
        for(sdlimb i = static_cast<sdlimb>(limbs.size()) - 2; i >= 0; i--)
        {
            std::string ch = std::to_string(limbs[i]);
            ch = std::string(BASEEXP - ch.size(), '0') + ch;
            digits += ch;
        }
        std::string result;
        if(scale > 0)
        {
            if(scale >= digits.size())
                result = "0." + std::string(scale - digits.size(), '0') + digits;
            else
                result = digits.substr(0, digits.size() - scale) + "." + digits.substr(digits.size() - scale);
        }
        else result = digits;
        if(isneg) result = "-" + result;
        if(scale > 0)
        {
            while(!result.empty() && result.back() == '0') result.pop_back();
            if(!result.empty() && result.back() == '.') result.pop_back();
        }
        return result;
    }
    BigDec ScaleTo(const BigDec& x, size_t NewScale)
    {
        if(x.scale >= NewScale) { BigDec r = x; r.scale = NewScale; return r; }
        BigDec r = x;
        if(IsZero(r.limbs)) { r.scale = NewScale; return r; }
        r.limbs = ScaleLimb(r.limbs, NewScale - r.scale);
        r.scale = NewScale;
        return r;
    }
    BigNum BigNum::operator-() const
    {
        BigNum r = *this;
        if(state == State::Inf) r.state = State::NegInf;
        else if(state == State::NegInf) r.state = State::Inf;
        else if(state == State::Normal && !IsZero(limbs)) r.isneg = !isneg;
        return r;
    }
    BigNum BigNum::operator+(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigNum(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state != b.state) return BigNum(State::Nan);
            return BigNum(state != State::Normal ? state : b.state);
        }
        long long x, y;
        if(scale == 0 && b.scale == 0 && toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y))
            return BigNum(x + y);
        const size_t smax = (std::max)(scale, b.scale);
        const std::vector<limb> al = ScaleLimb(limbs, smax - scale);
        const std::vector<limb> bl = ScaleLimb(b.limbs, smax - b.scale);
        BigNum res; res.state = State::Normal; res.scale = smax;
        if(isneg == b.isneg)
        {
            res.limbs = add(al, bl);
            res.isneg = isneg;
        }
        else
        {
            const int c = MagCmp(al, bl);
            if(c == 0) return BigNum(0);
            if(c > 0) { res.limbs = sub(al, bl); res.isneg = isneg; }
            else      { res.limbs = sub(bl, al); res.isneg = b.isneg; }
        }
        return res;
    }
    BigNum BigNum::operator-(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigNum(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state == b.state) return BigNum(State::Nan);
            if(state != State::Normal) return BigNum(state);
            return BigNum(b.state == State::Inf ? State::NegInf : State::Inf);
        }
        long long x, y;
        if(scale == 0 && b.scale == 0 && toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y))
            return BigNum(x - y);
        const size_t smax = (std::max)(scale, b.scale);
        const std::vector<limb> al = ScaleLimb(limbs, smax - scale);
        const std::vector<limb> bl = ScaleLimb(b.limbs, smax - b.scale);
        BigNum res; res.state = State::Normal; res.scale = smax;
        if(isneg != b.isneg)
        {
            res.limbs = add(al, bl);
            res.isneg = isneg;
        }
        else
        {
            const int c = MagCmp(al, bl);
            if(c == 0) return BigNum(0);
            if(c > 0) { res.limbs = sub(al, bl); res.isneg = isneg; }
            else      { res.limbs = sub(bl, al); res.isneg = !isneg; }
        }
        return res;
    }
    BigNum BigNum::operator*(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigNum(State::Nan);
        const bool ai = IsInf(), bi = b.IsInf();
        if(ai || bi)
        {
            const bool az = state == State::Normal && IsZero(limbs);
            const bool bz = b.state == State::Normal && IsZero(b.limbs);
            if((ai && bz) || (bi && az)) return BigNum(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            return BigNum((an ^ bn) ? State::NegInf : State::Inf);
        }
        BigNum res; res.state = State::Normal;
        res.limbs = mul(limbs, b.limbs);
        res.isneg = isneg ^ b.isneg;
        res.scale = scale + b.scale;
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    BigNum BigNum::operator/(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigNum(State::Nan);
        if(IsInf() || b.IsInf())
        {
            if(IsInf() && b.IsInf()) return BigNum(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            if(IsInf()) return BigNum((an ^ bn) ? State::NegInf : State::Inf);
            return BigNum(0);
        }
        if(IsZero(b.limbs))
        {
            KLOG_ERROR(KBIGNUM_DIVBYZERO, "divide by zero");
            return IsZero(limbs) ? BigNum(State::Nan)
                                 : BigNum((isneg ^ b.isneg) ? State::NegInf : State::Inf);
        }
        if(IsZero(limbs)) return BigNum(0);
        if(scale == 0 && b.scale == 0)
        {
            std::vector<limb> q, r;
            div(limbs, b.limbs, q, r);
            if(IsZero(r))
            {
                BigNum res; res.state = State::Normal; res.scale = 0;
                res.limbs = q;
                res.isneg = isneg ^ b.isneg;
                if(IsZero(res.limbs)) res.isneg = false;
                return res;
            }
        }
        const std::vector<limb> numer = ScaleLimb(limbs, b.scale + KEEP);
        std::vector<limb> q, r;
        div(numer, b.limbs, q, r);
        BigNum res; res.state = State::Normal;
        res.limbs = q;
        res.isneg = isneg ^ b.isneg;
        res.scale = scale + KEEP;
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    BigNum BigNum::operator%(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigNum(State::Nan);
        if(IsInf() || b.IsInf()) return BigNum(State::Nan);
        if(IsZero(b.limbs)) { KLOG_ERROR(KBIGNUM_DIVBYZERO, "module by zero"); return BigNum(State::Nan); }
        if(IsZero(limbs)) return BigNum(0);
        const size_t smax = (std::max)(scale, b.scale);
        const std::vector<limb> aa = ScaleLimb(limbs, smax - scale);
        const std::vector<limb> bb = ScaleLimb(b.limbs, smax - b.scale);
        std::vector<limb> q, r;
        div(aa, bb, q, r);
        BigNum res; res.state = State::Normal; res.scale = smax;
        res.limbs = r;
        res.isneg = isneg;
        if(IsZero(res.limbs)) res.isneg = false;
        return res;
    }
    bool BigNum::operator==(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state != State::Normal || b.state != State::Normal) return state == b.state;
        return CmpNum(*this, b) == 0;
    }
    bool BigNum::operator!=(const BigNum& b) const { return !(*this == b); }
    bool BigNum::operator<(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state == State::NegInf) return b.state != State::NegInf;
        if(b.state == State::Inf) return state != State::Inf;
        if(state == State::Inf) return false;
        if(b.state == State::NegInf) return false;
        return CmpNum(*this, b) < 0;
    }
    bool BigNum::operator<=(const BigNum& b) const { return (*this < b) || (*this == b); }
    bool BigNum::operator>(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this <= b);
    }
    bool BigNum::operator>=(const BigNum& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this < b);
    }
    BigNum::operator long long() const { return std::stoll(ToStr()); }
    BigNum::operator double() const { return std::stod(ToStr()); }
    bool BigNum::IsInLongLongRange() const
    {
        if(state != State::Normal) return false;
        long long dummy;
        return toLL(limbs, isneg, dummy);
    }
    bool BigNum::IsInDoubleRange() const { return sigDigits(ToStr()) <= 308; }

    BigDec BigDec::ToBig(const std::string& str) { return BigDec(BigNum::ToBig(str)); }
    BigDec BigDec::operator-() const { return BigDec(BigNum::operator-()); }

    BigInt BigGcd(BigInt a, BigInt b)
    {
        a.isneg = b.isneg = false;
        a.state = b.state = BigInt::State::Normal;
        while(b != 0)
        {
            BigInt r = a % b;
            a = b; b = r;
        }
        return a;
    }

    BigCpx<> Pow(const BigCpx<>& z, const BigDec& n)
    {
        if(n.IsNan() || z.re.IsNan() || z.im.IsNan()) return BigCpx<>(BigDec("nan"), BigDec("nan"));
        if(n.scale == 0)
        {
            BigInt e(n.ToStr());
            bool neg = false;
            if(e.isneg) { e.isneg = false; neg = true; }
            BigCpx<> result(BigDec(1), BigDec(0));
            BigCpx<> base = z;
            while(e != 0)
            {
                if(e % 2 == 1) result = result * base;
                base = base * base;
                e = e / BigInt(2);
            }
            if(neg) return BigCpx<>(1, 0) / result;
            return result;
        }
        const double rr = (double)z.re, ii = (double)z.im, ee = std::stod(n.ToStr());
        const double r = std::sqrt(rr * rr + ii * ii);
        const double th = std::atan2(ii, rr);
        const double mag = std::pow(r, ee);
        const BigDec R(mag * std::cos(ee * th));
        const BigDec I(mag * std::sin(ee * th));
        return BigCpx<>(R, I);
    }

    static BigDec IntQuot(const BigDec& a, const BigDec& b)
    {
        if(a.scale == 0 && b.scale == 0)
        {
            std::vector<limb> q, r;
            div(a.limbs, b.limbs, q, r);
            BigDec res; res.state = BigDec::State::Normal; res.scale = 0;
            res.limbs = q; res.isneg = a.isneg ^ b.isneg;
            if(IsZero(res.limbs)) res.isneg = false;
            return res;
        }
        return BigDec(std::stoll((a / b).ToStr()));
    }

    static BigDec RootReal(const BigDec& a, const BigDec& n);

    BigDec Pow(const BigDec& a, const BigDec& b)
    {
        if(b.IsNan())
        {
            BigDec absA = a; absA.isneg = false;
            return (absA == 1) ? BigDec(1) : BigDec(BigDec::State::Nan);
        }
        if(a.IsNan()) return BigDec(BigDec::State::Nan);
        if(a.IsInf())
        {
            if(b == 0) return BigDec(1);
            if(b.IsInf()) return b.state == BigDec::State::Inf ? BigDec(BigDec::State::Inf) : BigDec(0);
            if(b.isneg) return BigDec(0);
            const bool intExp = b.IsNormal() && b.scale == 0;
            if(a.state == BigDec::State::Inf) return BigDec(BigDec::State::Inf);
            if(intExp)
            {
                const bool odd = b.limbs.size() == 1 && b.limbs[0] % 2 == 1;
                return BigDec(odd ? BigDec::State::NegInf : BigDec::State::Inf);
            }
            return BigDec(BigDec::State::Nan);
        }
        if(b.IsInf())
        {
            BigDec absA = a; absA.isneg = false;
            const bool gt1 = (absA > 1), lt1 = (absA < 1);
            if(!gt1 && !lt1) return BigDec(1);
            if(b.state == BigDec::State::Inf) return gt1 ? BigDec(BigDec::State::Inf) : BigDec(0);
            return gt1 ? BigDec(0) : BigDec(BigDec::State::Inf);
        }
        if(IsZero(a.limbs))
        {
            if(b == 0) return BigDec(1);
            return b.isneg ? BigDec(BigDec::State::Inf) : BigDec(0);
        }
        if(b.isneg)
        {
            BigDec pos(b); pos.isneg = false;
            return BigDec(1) / Pow(a, pos);
        }
        const bool intExp = b.scale == 0;
        if(intExp)
        {
            BigDec result(1), base = a, exp = b;
            while(exp != 0)
            {
                if(exp % 2 == 1) result = result * base;
                base = base * base;
                exp = IntQuot(exp, BigDec(2));
            }
            return result;
        }
        BigDec p = b; p.scale = 0; p.isneg = false;
        std::string qs = std::string("1") + std::string(b.scale, '0');
        BigInt qB(qs);
        BigInt numI(p.ToStr());
        const BigInt g = BigGcd(numI, qB);
        if(g != 1) { numI = numI / g; qB = qB / g; }
        return Pow(RootReal(a, BigDec(qB.ToStr())), BigDec(numI.ToStr()));
    }

    static BigDec RootReal(const BigDec& a, const BigDec& n)
    {
        if(n.IsNormal() && IsZero(n.limbs)) return BigDec(BigDec::State::Nan);
        if(a.IsNan() || n.IsNan()) return BigDec(BigDec::State::Nan);
        if(n.isneg)
        {
            BigDec pos = n; pos.isneg = false;
            return BigDec(1) / RootReal(a, pos);
        }
        if(a.state == BigDec::State::Inf) return BigDec(BigDec::State::Inf);
        if(a.state == BigDec::State::NegInf)
        {
            if(n.IsNormal() && n.scale == 0 && n.limbs[0] % 2 == 1)
                return BigDec(BigDec::State::NegInf);
            return BigDec(BigDec::State::Nan);
        }
        if(IsZero(a.limbs)) return BigDec(0);
        if(!(n.IsNormal() && n.scale == 0)) return Pow(a, BigDec(1) / n);
        if(a.isneg)
        {
            if(n.limbs[0] % 2 == 0) return BigDec(BigDec::State::Nan);
            BigDec absA = a; absA.isneg = false;
            BigDec r = RootReal(absA, n);
            r.isneg = true;
            return r;
        }
        const long long nll = std::stoll(n.ToStr());
        const size_t DEC = KEEP + 1;
        const size_t m = static_cast<size_t>(nll) * DEC;
        BigDec A = ScaleTo(a, m); A.scale = 0;
        const size_t digits = A.ToStr().size();
        const BigDec N(nll);
        BigDec low(0);
        BigDec high(std::string("1") +
                    std::string((digits + static_cast<size_t>(nll)) / static_cast<size_t>(nll) + 1, '0'));
        while(low + 1 < high)
        {
            BigDec mid = IntQuot(low + high, BigDec(2));
            if(Pow(mid, N) <= A) low = mid;
            else                 high = mid;
        }
        BigDec X = low;
        BigDec Xr = IntQuot(X + 5, BigDec(10));
        Xr.scale = KEEP;
        Xr.isneg = false;
        return Xr;
    }

    BigCpx<> Root(const BigDec& a, const BigDec& n)
    {
        if(n.IsNormal() && IsZero(n.limbs)) return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
        if(a.IsNan() || n.IsNan())          return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
        if(n.isneg)
        {
            BigDec pos = n; pos.isneg = false;
            const bool imRoot = a.IsNormal() && a.isneg &&
                                pos.IsNormal() && pos.scale == 0 && pos.limbs[0] % 2 == 0;
            if(imRoot)
            {
                BigDec absA = a; absA.isneg = false;
                const BigDec b = RootReal(absA, pos);
                return BigCpx<>(BigDec(0), -(BigDec(1) / b));
            }
            return BigCpx<>(BigDec(1) / RootReal(a, pos), BigDec(0));
        }
        if(a.IsNormal() && a.isneg && n.IsNormal() && n.scale == 0 && n.limbs[0] % 2 == 0)
        {
            BigDec absA = a; absA.isneg = false;
            return BigCpx<>(BigDec(0), RootReal(absA, n));
        }
        return BigCpx<>(RootReal(a, n), BigDec(0));
    }

    BigInt RandBigInt(std::pair<size_t,size_t> IntRand, int sign)
    {
        if(IntRand.second < IntRand.first) std::swap(IntRand.first, IntRand.second);
        bool isneg;
        if(sign == 1) isneg = false;
        else if(sign == 2) isneg = true;
        else isneg = (bn_RandInt(0, 1) == 1);
        const size_t IntSize = IntRand.second == 0 ? 0 : bn_RandInt(IntRand.first, IntRand.second);
        if(IntSize == 0) return BigInt(0);
        BigInt res;
        res.limbs.clear(); res.isneg = isneg; res.state = BigInt::State::Normal;
        size_t wp = 0;
        while(wp < IntSize)
        {
            const size_t remain = IntSize - wp;
            const size_t genLen = (remain < BASEEXP) ? remain : BASEEXP;
            const sdlimb maxv = bn_Pow10(genLen) - 1;
            sdlimb num;
            if(wp + genLen == IntSize)
                num = bn_RandInt(bn_Pow10(genLen - 1), maxv);
            else
                num = bn_RandInt(0, maxv);
            res.limbs.push_back(static_cast<limb>(num));
            wp += genLen;
        }
        return res;
    }

    BigDec RandBigDec(std::pair<size_t,size_t> IntRand, std::pair<size_t,size_t> DecRand, int sign)
    {
        if(IntRand.second < IntRand.first) std::swap(IntRand.first, IntRand.second);
        if(DecRand.second < DecRand.first) std::swap(DecRand.first, DecRand.second);
        bool isneg;
        if(sign == 1) isneg = false;
        else if(sign == 2) isneg = true;
        else isneg = (bn_RandInt(0, 1) == 1);
        const size_t IntSize = IntRand.second == 0 ? 0 : bn_RandInt(IntRand.first, IntRand.second);
        const size_t DecSize = DecRand.second == 0 ? 0 : bn_RandInt(DecRand.first, DecRand.second);
        if(IntSize == 0 && DecSize == 0) return BigDec(0);
        BigDec res;
        res.limbs.clear(); res.isneg = isneg; res.scale = DecSize; res.state = BigDec::State::Normal;
        const size_t Total = IntSize + DecSize;
        size_t wp = 0;
        while(wp < Total)
        {
            const size_t remain = Total - wp;
            const size_t genLen = (remain < BASEEXP) ? remain : BASEEXP;
            const sdlimb maxv = bn_Pow10(genLen) - 1;
            sdlimb num;
            if(wp + genLen == Total && IntSize > 0)
                num = bn_RandInt(bn_Pow10(genLen - 1), maxv);
            else
                num = bn_RandInt(0, maxv);
            res.limbs.push_back(static_cast<limb>(num));
            wp += genLen;
        }
        return res;
    
    }
