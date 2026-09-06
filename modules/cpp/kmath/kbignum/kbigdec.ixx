module;
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cctype>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ostream>
#include <istream>
#include <type_traits>
#include <utility>
#include <random>
#include <iomanip>
#include <limits>

#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module kbignum.kbigdec;

import klogger;
import kbignum.kbigint;

export
{
    class BigDec;

    //===============================================================
    // 数学类型等级元函数：非数学=0，整数=1，十进制=2，分数=3，复数=4
    //===============================================================
    template<class T> struct MathRank { static constexpr int value = 0; };
    template<> struct MathRank<BigInt>{ static constexpr int value = 1; };
    template<> struct MathRank<BigDec>{ static constexpr int value = 2; };

    template<class T> struct IsMathType
    { static constexpr bool value = (MathRank<T>::value >= 1 && MathRank<T>::value <= 4); };

    class BigDec
    {
        public:
            enum class State { Normal, Inf, NegInf, Nan };

            std::vector<limb> limbs;   // 去掉小数点的整数值（base=2^32），值 = limbs / 10^scale
            bool isneg = false;
            size_t scale = 0;          // 十进制小数位数
            State state = State::Normal;

            bool IsInf()    const { return state == State::Inf  || state == State::NegInf; }
            bool IsNan()    const { return state == State::Nan; }
            bool IsNormal() const { return state == State::Normal; }
            std::string type() const;
            std::string ToStr() const;

            BigDec() = default;
            BigDec(const BigDec&) = default;
            BigDec& operator=(const BigDec&) = default;
            BigDec(const std::string& str);
            explicit BigDec(State s);
            explicit BigDec(long long v);
            explicit BigDec(const char* s): BigDec(std::string(s)) {}
            BigDec(const BigInt& bi);            // 整数 -> 十进制
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec(const T& num) { *this = FromArith(num); }

            BigDec operator+() const { return *this; }
            BigDec operator-() const;
            BigDec operator+(const BigDec& b) const;
            BigDec operator-(const BigDec& b) const;
            BigDec operator*(const BigDec& b) const;
            BigDec operator/(const BigDec& b) const;
            BigDec operator%(const BigDec& b) const;

            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec operator+(const T& n) const { return *this + BigDec(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec operator-(const T& n) const { return *this - BigDec(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec operator*(const T& n) const { return *this * BigDec(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec operator/(const T& n) const { return *this / BigDec(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigDec operator%(const T& n) const { return *this % BigDec(n); }

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

            bool operator==(const BigDec& b) const;
            bool operator!=(const BigDec& b) const;
            bool operator< (const BigDec& b) const;
            bool operator<=(const BigDec& b) const;
            bool operator> (const BigDec& b) const;
            bool operator>=(const BigDec& b) const;

            friend std::ostream& operator<<(std::ostream& os, const BigDec& b){ os << b.ToStr(); return os; }
            friend std::istream& operator>>(std::istream& is, BigDec& b){ std::string t; if(is >> t) b = BigDec(t); return is; }

            explicit operator long long() const;
            explicit operator double() const;
            bool IsInLongLongRange() const;
            bool IsInDoubleRange() const;

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

    // ---- 提升/转换基础设施 ----
    inline BigDec toBigDec(const BigDec& x);
    inline BigDec toBigDec(const BigInt& x);
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    inline BigDec toBigDec(const T& x);

    template<class T, class Enable = void> struct BigFromDec;
    template<> struct BigFromDec<BigDec>{ static BigDec from(const BigDec& d){ return d; } };
    template<> struct BigFromDec<BigInt>{ static BigInt from(const BigDec& d); };

    template<class T, class Enable = void> struct BigTraits
    {
        static bool is_zero(const T& v){ (void)v; return false; }
        static bool is_neg (const T& v){ (void)v; return false; }
        static T zero(){ return T(); }
        static T one (){ return T(1); }
    };
    template<class T> struct BigTraits<T, std::enable_if_t<std::is_arithmetic_v<T>>>
    {
        static bool is_zero(const T& v){ return v == 0; }
        static bool is_neg (const T& v){ return v < 0; }
        static T zero(){ return T(0); }
        static T one (){ return T(1); }
    };
    template<> struct BigTraits<BigDec>
    {
        static bool is_zero(const BigDec& v){ return v.state == BigDec::State::Normal && v.limbs.size() == 1 && v.limbs[0] == 0; }
        static bool is_neg (const BigDec& v){ return v.state == BigDec::State::Normal && v.isneg; }
        static BigDec zero(){ return BigDec("0"); }
        static BigDec one (){ return BigDec("1"); }
    };
    template<> struct BigTraits<BigInt>
    {
        static bool is_zero(const BigInt& v){ return v.state == BigInt::State::Normal && v.limbs.size() == 1 && v.limbs[0] == 0; }
        static bool is_neg (const BigInt& v){ return v.state == BigInt::State::Normal && v.isneg; }
        static BigInt zero(){ return BigInt(0); }
        static BigInt one (){ return BigInt(1); }
    };

    // ---- 自由函数前向声明（供 Pow 等互相引用） ----
    BigDec RootReal(const BigDec& a, const BigDec& n);
}

    //===============================================================
    // BigDec 实现（base=2^32 尾数 + 十进制 scale）
    //===============================================================
    namespace
    {
        inline bool bdZero(const std::vector<limb>& v){ return v.size() == 1 && v[0] == 0; }

        inline BigInt MagInt(std::vector<limb> m)
        {
            BigInt r; r.limbs = std::move(m); r.isneg = false;
            r.state = BigInt::State::Normal;
            return r;
        }

        // 尾数放大 10^k 倍
        std::vector<limb> scaleUpMag(const std::vector<limb>& m, size_t k)
        {
            if(k == 0) return m;
            BigInt p(std::string("1") + std::string(k, '0'));
            BigInt x = MagInt(m) * p;
            return x.limbs;
        }

        // 对齐（比较缩放前的最小公共精度）用于加减/取模
        void align(const BigDec& a, const BigDec& b, size_t smax,
                   std::vector<limb>& am, std::vector<limb>& bm)
        {
            am = scaleUpMag(a.limbs, smax - a.scale);
            bm = scaleUpMag(b.limbs, smax - b.scale);
        }
    }

    BigDec::BigDec(const std::string& str)
    {
        std::string s = str;
        size_t b = 0, e = s.size();
        while(b < e && isspace((unsigned char)s[b])) ++b;
        while(e > b && isspace((unsigned char)s[e-1])) --e;
        for(size_t i = b; i < e; ++i) s[i] = (char)std::tolower((unsigned char)s[i]);
        std::string tok = s.substr(b, e - b);
        if(tok == "inf" || tok == "+inf"){ state = State::Inf;    return; }
        if(tok == "-inf"){ state = State::NegInf; return; }
        if(tok == "nan"){ state = State::Nan;    return; }
        // 普通十进制
        size_t dot = tok.find('.');
        scale = (dot == std::string::npos) ? 0 : (tok.size() - dot - 1);
        isneg = (!tok.empty() && tok[0] == '-');
        std::string digits;
        for(size_t i = 0; i < tok.size(); ++i)
            if(std::isdigit((unsigned char)tok[i])) digits += tok[i];
        if(digits.empty()) digits = "0";
        BigInt m(digits);
        limbs = m.limbs;
        if(bdZero(limbs)) isneg = false;
    }

    BigDec::BigDec(State s){ state = s; isneg = (s == State::NegInf); limbs = {0}; scale = 0; }
    BigDec::BigDec(long long v){ *this = BigDec(std::to_string(v)); }
    BigDec::BigDec(const BigInt& bi){ *this = BigDec(bi.ToStr()); }

    std::string BigDec::type() const
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

    std::string BigDec::ToStr() const
    {
        if(state == State::Nan) return "nan";
        if(state == State::Inf) return "inf";
        if(state == State::NegInf) return "-inf";
        if(bdZero(limbs)) return "0";
        BigInt m; m.limbs = limbs; m.isneg = false; m.state = BigInt::State::Normal;
        const std::string digits = m.ToStr();
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

    BigDec BigDec::operator-() const
    {
        if(state == State::Inf) return BigDec(State::NegInf);
        if(state == State::NegInf) return BigDec(State::Inf);
        BigDec r = *this;
        if(!bdZero(limbs)) r.isneg = !isneg;
        return r;
    }

    BigDec BigDec::operator+(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigDec(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state != b.state) return BigDec(State::Nan);
            return BigDec(state != State::Normal ? state : b.state);
        }
        const size_t smax = (std::max)(scale, b.scale);
        std::vector<limb> al, bl;
        align(*this, b, smax, al, bl);
        BigDec res; res.state = State::Normal; res.scale = smax;
        if(isneg == b.isneg)
        {
            BigInt s = MagInt(std::move(al)) + MagInt(std::move(bl));
            res.limbs = s.limbs; res.isneg = isneg;
        }
        else
        {
            BigInt A = MagInt(std::move(al)), B = MagInt(std::move(bl));
            if(A == B) return BigDec("0");
            if(A > B){ res.limbs = (A - B).limbs; res.isneg = isneg; }
            else     { res.limbs = (B - A).limbs; res.isneg = b.isneg; }
        }
        if(bdZero(res.limbs)) res.isneg = false;
        return res;
    }

    BigDec BigDec::operator-(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigDec(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state == b.state) return BigDec(State::Nan);
            if(state != State::Normal) return BigDec(state);
            return BigDec(b.state == State::Inf ? State::NegInf : State::Inf);
        }
        const size_t smax = (std::max)(scale, b.scale);
        std::vector<limb> al, bl;
        align(*this, b, smax, al, bl);
        BigDec res; res.state = State::Normal; res.scale = smax;
        if(isneg != b.isneg)
        {
            BigInt s = MagInt(std::move(al)) + MagInt(std::move(bl));
            res.limbs = s.limbs; res.isneg = isneg;
        }
        else
        {
            BigInt A = MagInt(std::move(al)), B = MagInt(std::move(bl));
            if(A == B) return BigDec("0");
            if(A > B){ res.limbs = (A - B).limbs; res.isneg = isneg; }
            else     { res.limbs = (B - A).limbs; res.isneg = !isneg; }
        }
        if(bdZero(res.limbs)) res.isneg = false;
        return res;
    }

    BigDec BigDec::operator*(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigDec(State::Nan);
        const bool ai = IsInf(), bi = b.IsInf();
        if(ai || bi)
        {
            const bool az = state == State::Normal && bdZero(limbs);
            const bool bz = b.state == State::Normal && bdZero(b.limbs);
            if((ai && bz) || (bi && az)) return BigDec(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            return BigDec((an ^ bn) ? State::NegInf : State::Inf);
        }
        BigInt s = MagInt(limbs) * MagInt(b.limbs);
        BigDec res; res.state = State::Normal;
        res.limbs = s.limbs;
        res.isneg = isneg ^ b.isneg;
        res.scale = scale + b.scale;
        if(bdZero(res.limbs)) res.isneg = false;
        return res;
    }

    BigDec BigDec::operator/(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigDec(State::Nan);
        if(IsInf() || b.IsInf())
        {
            if(IsInf() && b.IsInf()) return BigDec(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            if(IsInf()) return BigDec((an ^ bn) ? State::NegInf : State::Inf);
            return BigDec("0");
        }
        if(bdZero(b.limbs))
        {
            KLOG_ERROR(KBIGNUM_DIVBYZERO, "divide by zero");
            return bdZero(limbs) ? BigDec(State::Nan)
                : BigDec((isneg ^ b.isneg) ? State::NegInf : State::Inf);
        }
        if(bdZero(limbs)) return BigDec("0");
        // 精确保留 KEEP 位小数：numer = a * 10^(b.scale+KEEP)，商整数商，scale = a.scale + KEEP
        BigInt numer = MagInt(limbs) * BigInt(std::string("1") + std::string(b.scale + KBIGNUM_KEEP, '0'));
        BigInt den = MagInt(b.limbs);
        BigInt q = numer / den;
        BigDec res; res.state = State::Normal;
        res.limbs = q.limbs;
        res.isneg = isneg ^ b.isneg;
        res.scale = scale + KBIGNUM_KEEP;
        if(bdZero(res.limbs)) res.isneg = false;
        return res;
    }

    BigDec BigDec::operator%(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigDec(State::Nan);
        if(IsInf() || b.IsInf()) return BigDec(State::Nan);
        if(bdZero(b.limbs)){ KLOG_ERROR(KBIGNUM_DIVBYZERO, "module by zero"); return BigDec(State::Nan); }
        if(bdZero(limbs)) return BigDec("0");
        const size_t smax = (std::max)(scale, b.scale);
        std::vector<limb> al, bl;
        align(*this, b, smax, al, bl);
        BigInt r = MagInt(std::move(al)) % MagInt(std::move(bl));
        BigDec res; res.state = State::Normal; res.scale = smax;
        res.limbs = r.limbs; res.isneg = isneg;
        if(bdZero(res.limbs)) res.isneg = false;
        return res;
    }

    bool BigDec::operator==(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state != State::Normal || b.state != State::Normal) return state == b.state;
        if(isneg != b.isneg)
        {
            if(bdZero(limbs) && bdZero(b.limbs)) return true;
            return false;
        }
        const size_t smax = (std::max)(scale, b.scale);
        std::vector<limb> al, bl; align(*this, b, smax, al, bl);
        BigInt A = MagInt(std::move(al)), B = MagInt(std::move(bl));
        return A == B;
    }
    bool BigDec::operator!=(const BigDec& b) const { return !(*this == b); }
    bool BigDec::operator<(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state == State::NegInf) return b.state != State::NegInf;
        if(b.state == State::Inf) return state != State::Inf;
        if(state == State::Inf) return false;
        if(b.state == State::NegInf) return false;
        if(isneg != b.isneg)
        {
            if(bdZero(limbs) && bdZero(b.limbs)) return false;
            return isneg;
        }
        const size_t smax = (std::max)(scale, b.scale);
        std::vector<limb> al, bl; align(*this, b, smax, al, bl);
        BigInt A = MagInt(std::move(al)), B = MagInt(std::move(bl));
        const bool r = (A < B);
        return isneg ? !r : r;
    }
    bool BigDec::operator<=(const BigDec& b) const { return (*this < b) || (*this == b); }
    bool BigDec::operator>(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this <= b);
    }
    bool BigDec::operator>=(const BigDec& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        return !(*this < b);
    }

    BigDec::operator long long() const { return std::stoll(ToStr()); }
    BigDec::operator double() const { return std::stod(ToStr()); }
    bool BigDec::IsInLongLongRange() const
    {
        if(state != State::Normal) return false;
        BigInt m; m.limbs = limbs; m.isneg = isneg; m.state = BigInt::State::Normal;
        return m.IsInLongLongRange();
    }
    bool BigDec::IsInDoubleRange() const { return ToStr().size() <= 340; }

    inline BigDec toBigDec(const BigDec& x){ return x; }
    inline BigDec toBigDec(const BigInt& x){ return BigDec(x); }
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
    inline BigDec toBigDec(const T& x){ return BigDec(x); }

    BigInt BigFromDec<BigInt>::from(const BigDec& d){ return BigInt(d.ToStr()); }

    //===============================================================
    // 自由函数（BigDec 相关）
    //===============================================================
    namespace
    {
        std::random_device ddec_rd;
        std::mt19937 ddec_gen(ddec_rd());
        sdlimb ddec_rand(sdlimb a, sdlimb b){ std::uniform_int_distribution<sdlimb> d(a, b); return d(ddec_gen); }

        BigDec IntQuotBD(const BigDec& a, const BigDec& b)
        {
            BigInt A(a.ToStr()), B(b.ToStr());
            return BigDec((A / B).ToStr());
        }
    }

    export BigDec ScaleTo(const BigDec& x, size_t NewScale)
    {
        if(x.scale >= NewScale){ BigDec r = x; r.scale = NewScale; return r; }
        BigDec r = x;
        if(r.limbs.size() == 1 && r.limbs[0] == 0){ r.scale = NewScale; return r; }
        r.limbs = scaleUpMag(r.limbs, NewScale - r.scale);
        r.scale = NewScale;
        return r;
    }

    export BigDec Pow(const BigDec& a, const BigDec& b)
    {
        if(b.IsNan())
        {
            BigDec absA = a; absA.isneg = false;
            return (absA == BigDec(1)) ? BigDec(1) : BigDec(BigDec::State::Nan);
        }
        if(a.IsNan()) return BigDec(BigDec::State::Nan);
        if(a.IsInf())
        {
            if(b == BigDec(0)) return BigDec(1);
            if(b.IsInf()) return b.state == BigDec::State::Inf ? BigDec(BigDec::State::Inf) : BigDec("0");
            if(b.isneg) return BigDec("0");
            const bool intExp = b.IsNormal() && b.scale == 0;
            if(a.state == BigDec::State::Inf) return BigDec(BigDec::State::Inf);
            if(intExp){ const bool odd = b.limbs.size() == 1 && b.limbs[0] % 2 == 1; return BigDec(odd ? BigDec::State::NegInf : BigDec::State::Inf); }
            return BigDec(BigDec::State::Nan);
        }
        if(b.IsInf())
        {
            BigDec absA = a; absA.isneg = false;
            const bool gt1 = (absA > BigDec(1)), lt1 = (absA < BigDec(1));
            if(!gt1 && !lt1) return BigDec(1);
            if(b.state == BigDec::State::Inf) return gt1 ? BigDec(BigDec::State::Inf) : BigDec("0");
            return gt1 ? BigDec("0") : BigDec(BigDec::State::Inf);
        }
        if(bdZero(a.limbs))
        {
            if(b == BigDec(0)) return BigDec(1);
            return b.isneg ? BigDec(BigDec::State::Inf) : BigDec("0");
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
            while(exp != BigDec(0))
            {
                if(exp % BigDec(2) == BigDec(1)) result = result * base;
                base = base * base;
                exp = IntQuotBD(exp, BigDec(2));
            }
            return result;
        }
        BigDec p = b; p.scale = 0; p.isneg = false;
        std::string qs = std::string("1") + std::string(b.scale, '0');
        BigInt qB(qs);
        BigInt numI(p.ToStr());
        const BigInt g = BigGcd(numI, qB);
        if(g != BigInt(1)){ numI = numI / g; qB = qB / g; }
        return Pow(RootReal(a, BigDec(qB.ToStr())), BigDec(numI.ToStr()));
    }

    export BigDec RootReal(const BigDec& a, const BigDec& n)
    {
        if(n.IsNormal() && n.limbs.size() == 1 && n.limbs[0] == 0) return BigDec(BigDec::State::Nan);
        if(a.IsNan() || n.IsNan()) return BigDec(BigDec::State::Nan);
        if(n.isneg){ BigDec pos = n; pos.isneg = false; return BigDec(1) / RootReal(a, pos); }
        if(a.state == BigDec::State::Inf) return BigDec(BigDec::State::Inf);
        if(a.state == BigDec::State::NegInf)
        {
            if(n.IsNormal() && n.scale == 0 && n.limbs[0] % 2 == 1) return BigDec(BigDec::State::NegInf);
            return BigDec(BigDec::State::Nan);
        }
        if(bdZero(a.limbs)) return BigDec("0");
        if(!(n.IsNormal() && n.scale == 0)) return Pow(a, BigDec(1) / n);
        if(a.isneg)
        {
            if(n.limbs[0] % 2 == 0) return BigDec(BigDec::State::Nan);
            BigDec absA = a; absA.isneg = false;
            BigDec r = RootReal(absA, n); r.isneg = true; return r;
        }
        const long long nll = std::stoll(n.ToStr());
        const size_t DEC = KBIGNUM_KEEP + 1;
        const size_t m = static_cast<size_t>(nll) * DEC;
        BigDec A = ScaleTo(a, m); A.scale = 0;
        const size_t digits = A.ToStr().size();
        const BigDec N(nll);
        BigDec low("0");
        BigDec high(std::string("1") + std::string((digits + static_cast<size_t>(nll)) / static_cast<size_t>(nll) + 1, '0'));
        while(low + BigDec(1) < high)
        {
            BigDec mid = IntQuotBD(low + high, BigDec(2));
            if(Pow(mid, N) <= A) low = mid; else high = mid;
        }
        BigDec X = low;
        BigDec Xr = IntQuotBD(X + BigDec(5), BigDec(10));
        Xr.scale = KBIGNUM_KEEP;
        Xr.isneg = false;
        return Xr;
    }

    export BigDec RandBigDec(std::pair<size_t,size_t> IntRand = {0,0},
                             std::pair<size_t,size_t> DecRand = {0,0}, int sign = 0)
    {
        if(IntRand.second < IntRand.first) std::swap(IntRand.first, IntRand.second);
        if(DecRand.second < DecRand.first) std::swap(DecRand.first, DecRand.second);
        bool isneg;
        if(sign == 1) isneg = false;
        else if(sign == 2) isneg = true;
        else isneg = (ddec_rand(0, 1) == 1);
        const size_t ni = IntRand.second == 0 ? 0 : (size_t)ddec_rand((sdlimb)IntRand.first, (sdlimb)IntRand.second);
        const size_t nd = DecRand.second == 0 ? 0 : (size_t)ddec_rand((sdlimb)DecRand.first, (sdlimb)DecRand.second);
        if(ni == 0 && nd == 0) return isneg ? BigDec("-0") : BigDec(0);
        std::string s = isneg ? "-" : "";
        if(ni == 0) s += "0";
        else
        {
            for(size_t i = 0; i < ni; ++i) s.push_back('0' + ddec_rand(i == 0 ? 1 : 0, 9));
        }
        if(nd > 0)
        {
            s += ".";
            for(size_t i = 0; i < nd; ++i) s.push_back('0' + ddec_rand(0, 9));
        }
        return BigDec(s);
    }

    export inline BigDec RandBigNum(std::pair<size_t,size_t> i = {0,0},
                                    std::pair<size_t,size_t> d = {0,0}, int s = 0)
    { return RandBigDec(i, d, s); }