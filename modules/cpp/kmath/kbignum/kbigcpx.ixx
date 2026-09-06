module;
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <iostream>
#include <ostream>
#include <istream>

export module kbignum.kbigcpx;

import klogger;
import kbignum.kbigint;
import kbignum.kbigdec;
import kbignum.kbigfrc;

export
{
    template<class R = BigDec, class I = BigDec> class BigCpx;

    // 等级：复数 = 4
    template<class R, class I> struct MathRank<BigCpx<R,I>>
    { static constexpr int value = 4; };

    /// 复数 -> 十进制（取实部）
    template<class R, class I> BigDec toBigDec(const BigCpx<R,I>& x);

    template<class R, class I> struct BigFromDec<BigCpx<R,I>>
    { static BigCpx<R,I> from(const BigDec& d){ return BigCpx<R,I>(d); } };

    template<class R, class I> struct BigTraits<BigCpx<R,I>>
    {
        static bool is_zero(const BigCpx<R,I>& v)
        { return BigTraits<R>::is_zero(v.re) && BigTraits<I>::is_zero(v.im); }
        static bool is_neg (const BigCpx<R,I>& v){ return BigTraits<R>::is_neg(v.re); }
        static BigCpx<R,I> zero(){ return BigCpx<R,I>(BigTraits<R>::zero(), BigTraits<I>::zero()); }
        static BigCpx<R,I> one (){ return BigCpx<R,I>(BigTraits<R>::one(),  BigTraits<I>::zero()); }
    };

    /// 复数：实部 R / 虚部 I 可任意嵌套
    template<class R, class I>
    class BigCpx
    {
        public:
            R re;
            I im;
            std::string type() const { return "cpx"; }

            BigCpx() : re(BigTraits<R>::zero()), im(BigTraits<I>::zero()) {}
            BigCpx(const R& r, const I& i) : re(r), im(i) {}
            explicit BigCpx(const BigDec& d)
            : re(BigFromDec<R>::from(d)), im(BigTraits<I>::zero()) {}
            template<class T, std::enable_if_t<IsMathType<T>::value && !std::is_same_v<T,BigCpx<R,I>> && !std::is_same_v<T,BigDec>, int> = 0>
            BigCpx(const T& x)
            : re(BigFromDec<R>::from(toBigDec(x))), im(BigTraits<I>::zero()) {}
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigCpx(const T& num)
            : re(BigFromDec<R>::from(BigDec(num))), im(BigTraits<I>::zero()) {}

            BigCpx Conj() const { return BigCpx(re, -im); }
            R Abs() const
            {
                const BigDec s = toBigDec(re) * toBigDec(re) + toBigDec(im) * toBigDec(im);
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
            {
                return BigCpx(BigFromDec<R>::from(toBigDec(re*b.re) - toBigDec(im*b.im)),
                              BigFromDec<I>::from(toBigDec(re*b.im) + toBigDec(im*b.re)));
            }
            BigCpx operator/(const BigCpx& b) const
            {
                const BigDec den = toBigDec(b.re) * toBigDec(b.re) + toBigDec(b.im) * toBigDec(b.im);
                if(den.state == BigDec::State::Normal && den.limbs.size() == 1 && den.limbs[0] == 0)
                    return BigCpx(BigFromDec<R>::from(BigDec(BigDec::State::Nan)),
                                  BigFromDec<I>::from(BigDec(BigDec::State::Nan)));
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

    template<class R, class I>
    BigDec toBigDec(const BigCpx<R,I>& x){ return toBigDec(x.re); }

    // ---- BigCpx 自由函数 ----
    inline BigCpx<> Conj(const BigCpx<>& z){ return z.Conj(); }
    inline BigDec  Abs(const BigCpx<>& z){ return z.Abs(); }

    /// 复数幂 z^n
    BigCpx<> Pow(const BigCpx<>& z, const BigDec& n);
    inline BigCpx<> Pow(const BigCpx<>& z, const BigInt& n){ return Pow(z, BigDec(n)); }

    /// 开 n 次方：实数 → 实轴；负数偶次方 → 纯虚数
    BigCpx<> Root(const BigDec& a, const BigDec& n);
}

    namespace
    {
        inline bool bd_cpx_iszero(const BigDec& v)
        { return v.limbs.size() == 1 && v.limbs[0] == 0; }
    }

    BigCpx<> Pow(const BigCpx<>& z, const BigDec& n)
    {
        if(n.IsNan()) return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
        if(bd_cpx_iszero(n)) return BigCpx<>(BigDec(1), 0);
        BigInt e(n.ToStr());          // 取整数值
        const bool neg = e.isneg;
        e.isneg = false;
        BigCpx<> base = z, result(1, 0);
        while(!(e.limbs.size() == 1 && e.limbs[0] == 0))
        {
            if(e.limbs[0] % 2 == 1) result = result * base;
            base = base * base;
            e = e / BigInt(2);
        }
        if(neg) result = BigCpx<>(BigDec(1), 0) / result;
        return result;
    }

    BigCpx<> Root(const BigDec& a, const BigDec& n)
    {
        if(n.IsNormal() && n.limbs.size() == 1 && n.limbs[0] == 0)
            return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
        if(a.IsNan() || n.IsNan()) return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
        if(n.isneg)
        {
            BigDec pos = n; pos.isneg = false;
            const bool imRoot = a.IsNormal() && a.isneg
                && pos.IsNormal() && pos.scale == 0 && pos.limbs[0] % 2 == 0;
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