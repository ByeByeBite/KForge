module;
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <iostream>
#include <ostream>
#include <istream>

export module kbignum.kbigfrc;

import klogger;
import kbignum.kbigint;
import kbignum.kbigdec;

export
{
    template<class N = BigInt, class D = BigInt> class BigFrc;

    // 等级：分数 = 3
    template<class N, class D> struct MathRank<BigFrc<N,D>>{ static constexpr int value = 3; };

    /// 分数转十进制
    template<class N, class D> BigDec toBigDec(const BigFrc<N,D>& x);

    template<class N, class D> struct BigFromDec<BigFrc<N,D>>
    { static BigFrc<N,D> from(const BigDec& d){ return BigFrc<N,D>(d); } };

    template<class N, class D> struct BigTraits<BigFrc<N,D>>
    {
        static bool is_zero(const BigFrc<N,D>& v){ return BigTraits<N>::is_zero(v.numerator); }
        static bool is_neg (const BigFrc<N,D>& v){ return BigTraits<N>::is_neg(v.numerator); }
        static BigFrc<N,D> zero(){ return BigFrc<N,D>(BigTraits<N>::zero(), BigTraits<D>::one()); }
        static BigFrc<N,D> one (){ return BigFrc<N,D>(BigTraits<N>::one(),  BigTraits<D>::one()); }
    };

    template<class N, class D>
    class BigFrc
    {
        public:
            N numerator;
            D denominator;

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
                const bool z = d.limbs.size() == 1 && d.limbs[0] == 0;
                if(z){ numerator = BigTraits<N>::zero(); denominator = BigTraits<D>::one(); return; }
                BigDec intpart(d); intpart.scale = 0;
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

    template<class N, class D>
    BigDec toBigDec(const BigFrc<N,D>& x){ return x.ToBigDec(); }
}