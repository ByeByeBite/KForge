#include "modules/cpp/KF.hpp"
/**
 * @file KMATH.cpp
 * @brief 大数模块：共享量级核心 BigNum 基类 + 派生四类 BigInt/BigDec/BigFrc/BigCpx
 *        Root 支持虚数（负数开偶次方返回 BigCpx 虚部）
 * @version 2.1.0
 * @date 2026-08-30
 * @author Git-1145
**/
namespace KF
{
    namespace KMATH
    {
        //===============================================================
        //  内部 limb 原语（base=1e9，小端，裁剪高位零）——被四类复用
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

            std::vector<limb> sub(const std::vector<limb>& a, const std::vector<limb>& b) // |a| >= |b|
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

            // 乘以 10^k（k>=0）
            std::vector<limb> ScaleLimb(const std::vector<limb>& v, size_t k)
            {
                if(k == 0) return v;
                std::vector<limb> r = v;
                const size_t hi = k / BASEEXP, lo = k % BASEEXP;
                if(lo)
                {
                    const dlimb mult = KUTIL::Pow10(lo);
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

            // 长除法（无符号）：a / b，商与余数
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
                    slimb lo = 0, hi = static_cast<slimb>(BASE); // [lo,hi)
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

            // limb 向量 → long long（失败返回 false）
            bool toLL(const std::vector<limb>& v, bool neg, long long& out)
            {
                if(v.size() > 2) return false;
                unsigned long long mag = v[0];
                if(v.size() == 2) mag += (unsigned long long)v[1] * BASE;
                if(mag > 9223372036854775807ULL) return false;
                out = neg ? -static_cast<long long>(mag) : static_cast<long long>(mag);
                return true;
            }

            // 十进制有效数字个数（用于判断 double 范围）
            size_t sigDigits(const std::string& s)
            {
                size_t n = 0;
                for(char c : s)
                    if(c >= '0' && c <= '9') n++;
                return n;
            }
        }

        //===============================================================
        //  共享的带符号比较助手（BigInt / BigDec）
        //===============================================================
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

        //===============================================================
        //  Normalize：合法化（去小数点 去前后导0）
        //===============================================================
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

        //===============================================================
        //  BigInt
        //===============================================================
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

        BigInt::BigInt(const BigNum& b) // 十进制 → 取整（截断小数部分）
        {
            *this = BigInt(b.ToStr());
        }

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
            if(toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y)) return BigInt(x + y); // native
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
            if(toLL(limbs, isneg, x) && toLL(b.limbs, b.isneg, y)) return BigInt(x - y); // native
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
                KLOG_ERROR(KMATH_DIVBYZERO, "divide by zero");
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
            if(IsZero(b.limbs)) { KLOG_ERROR(KMATH_DIVBYZERO, "module by zero"); return BigInt(State::Nan); }
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

        //===============================================================
        //  BigNum（十进制核心基类：BigDec/BigInt/Frc/Cpx 的共享量级核心）
        //===============================================================
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
                return BigNum(x + y); // native
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
                return BigNum(x - y); // native
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
                KLOG_ERROR(KMATH_DIVBYZERO, "divide by zero");
                return IsZero(limbs) ? BigNum(State::Nan)
                                     : BigNum((isneg ^ b.isneg) ? State::NegInf : State::Inf);
            }
            if(IsZero(limbs)) return BigNum(0);
            // 整除检测：a、b 均为整数且能整除 → 整数
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
            // 否则保留 9 位小数：被除数放大 10^(b.scale+keep)
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
            if(IsZero(b.limbs)) { KLOG_ERROR(KMATH_DIVBYZERO, "module by zero"); return BigNum(State::Nan); }
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

        //===============================================================
        //  BigDec（薄封装：算术/比较/转换全部复用基类 BigNum，仅保返回类型）
        //===============================================================
        BigDec BigDec::ToBig(const std::string& str) { return BigDec(BigNum::ToBig(str)); }
        BigDec BigDec::operator-() const { return BigDec(BigNum::operator-()); }

        //===============================================================
        //  BigGcd（非负整数） / Pow / Root（供 BigDec 与小数指数使用）
        //===============================================================
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

        //===============================================================
        //  BigFrc / BigCpx（模板化，全部内联定义于 KF.hpp；此处仅保留自由函数 Pow）
        //===============================================================
        BigCpx<> Pow(const BigCpx<>& z, const BigDec& n)
        {
            if(n.IsNan() || z.re.IsNan() || z.im.IsNan()) return BigCpx<>(BigDec("nan"), BigDec("nan"));
            if(n.scale == 0) // 整数指数：精确重复平方
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
            // 小数指数：极坐标 double 近似
            const double rr = (double)z.re, ii = (double)z.im, ee = std::stod(n.ToStr());
            const double r = std::sqrt(rr * rr + ii * ii);
            const double th = std::atan2(ii, rr);
            const double mag = std::pow(r, ee);
            const BigDec R(mag * std::cos(ee * th));
            const BigDec I(mag * std::sin(ee * th));
            return BigCpx<>(R, I);
        }

        //===============================================================
        //  Pow / Root（Root 对外返回 BigCpx；RootReal 为内部实数根，供 Pow 小数指数）
        //===============================================================
        static BigDec IntQuot(const BigDec& a, const BigDec& b) // 截断向零的整数商
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
            return BigDec(std::stoll((a / b).ToStr())); // 一般情形回退
        }

        static BigDec RootReal(const BigDec& a, const BigDec& n); // 实数根（负数偶次→nan），供 Pow/Root 复用

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
            // 分数指数：a^b = (a^(1/q))^p
            BigDec p = b; p.scale = 0; p.isneg = false;
            std::string qs = std::string("1") + std::string(b.scale, '0');
            BigInt qB(qs);
            // 约分
            BigInt numI(p.ToStr());
            const BigInt g = BigGcd(numI, qB);
            if(g != 1) { numI = numI / g; qB = qB / g; }
            return Pow(RootReal(a, BigDec(qB.ToStr())), BigDec(numI.ToStr()));
        }

        //===============================================================
        //  RootReal：实数 n 次方根（BigDec 圆 9 位小数）。
        //  负数开偶次方 → nan；非整数指数 → 经 Pow(a, 1/n) 回退。
        //  仅供内部 Pow 小数指数路径与 Root(虚数) 复用。
        //===============================================================
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

        //===============================================================
        //  Root：对外 n 次方根，统一返回 BigCpx。
        //  负数开偶次方 → 纯虚数（实部 0，虚部 = |a|^(1/n)），如 √(-4)=2i。
        //===============================================================
        BigCpx<> Root(const BigDec& a, const BigDec& n)
        {
            if(n.IsNormal() && IsZero(n.limbs)) return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
            if(a.IsNan() || n.IsNan())          return BigCpx<>(BigDec(BigDec::State::Nan), BigDec(0));
            if(n.isneg)
            {
                // a^(1/n) = 1 / a^(1/|n|)；实数根用 BigDec 倒数(保留9位)，纯虚数根取共轭倒数 -i/b
                BigDec pos = n; pos.isneg = false;
                const bool imRoot = a.IsNormal() && a.isneg &&
                                    pos.IsNormal() && pos.scale == 0 && pos.limbs[0] % 2 == 0;
                if(imRoot)
                {
                    BigDec absA = a; absA.isneg = false;
                    const BigDec b = RootReal(absA, pos);
                    return BigCpx<>(BigDec(0), -(BigDec(1) / b)); // 1/(bi)= -i/b
                }
                return BigCpx<>(BigDec(1) / RootReal(a, pos), BigDec(0));
            }
            // 负数开偶次方 → 纯虚数
            if(a.IsNormal() && a.isneg && n.IsNormal() && n.scale == 0 && n.limbs[0] % 2 == 0)
            {
                BigDec absA = a; absA.isneg = false;
                return BigCpx<>(BigDec(0), RootReal(absA, n));
            }
            return BigCpx<>(RootReal(a, n), BigDec(0));
        }

        //===============================================================
        //  随机数
        //===============================================================
        BigInt RandBigInt(std::pair<size_t,size_t> IntRand, int sign)
        {
            if(IntRand.second < IntRand.first) std::swap(IntRand.first, IntRand.second);
            bool isneg;
            if(sign == 1) isneg = false;
            else if(sign == 2) isneg = true;
            else isneg = (KUTIL::RandInt(0, 1) == 1);
            const size_t IntSize = IntRand.second == 0 ? 0 : KUTIL::RandInt(IntRand.first, IntRand.second);
            if(IntSize == 0) return BigInt(0);
            BigInt res;
            res.limbs.clear(); res.isneg = isneg; res.state = BigInt::State::Normal;
            size_t wp = 0;
            while(wp < IntSize)
            {
                const size_t remain = IntSize - wp;
                const size_t genLen = (remain < BASEEXP) ? remain : BASEEXP;
                const sdlimb maxv = KUTIL::Pow10(genLen) - 1;
                sdlimb num;
                if(wp + genLen == IntSize)
                    num = KUTIL::RandInt(KUTIL::Pow10(genLen - 1), maxv);
                else
                    num = KUTIL::RandInt(0, maxv);
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
            else isneg = (KUTIL::RandInt(0, 1) == 1);
            const size_t IntSize = IntRand.second == 0 ? 0 : KUTIL::RandInt(IntRand.first, IntRand.second);
            const size_t DecSize = DecRand.second == 0 ? 0 : KUTIL::RandInt(DecRand.first, DecRand.second);
            if(IntSize == 0 && DecSize == 0) return BigDec(0);
            BigDec res;
            res.limbs.clear(); res.isneg = isneg; res.scale = DecSize; res.state = BigDec::State::Normal;
            const size_t Total = IntSize + DecSize;
            size_t wp = 0;
            while(wp < Total)
            {
                const size_t remain = Total - wp;
                const size_t genLen = (remain < BASEEXP) ? remain : BASEEXP;
                const sdlimb maxv = KUTIL::Pow10(genLen) - 1;
                sdlimb num;
                if(wp + genLen == Total && IntSize > 0)
                    num = KUTIL::RandInt(KUTIL::Pow10(genLen - 1), maxv);
                else
                    num = KUTIL::RandInt(0, maxv);
                res.limbs.push_back(static_cast<limb>(num));
                wp += genLen;
            }
            return res;
        }
    }
}
