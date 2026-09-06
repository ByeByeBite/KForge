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
#include <random>
#include <ostream>
#include <istream>
#include <algorithm>
#include <type_traits>
#include <utility>
#ifdef __AVX2__
#include <immintrin.h>
#endif

#define KLOG_ERROR(code, extra)   Error(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_WARNING(code, extra) Warning(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_INFO(code, extra)    Info(code, extra, __FILE__, __LINE__, __FUNCTION__)
#define KLOG_FATAL(code, extra)   Fatal(code, extra, __FILE__, __LINE__, __FUNCTION__)

export module kbignum.kbigint;

import klogger;

export
{
    using limb  = uint32_t;
    using dlimb = uint64_t;
    using slimb = int32_t;
    using sdlimb = int64_t;

    /// 除法等保留的十进制精度位数
    constexpr size_t KBIGNUM_KEEP = 9;

    //===============================================================
    // 算法选择（运行时枚举 + 自动阈值）
    //===============================================================
    enum class AddAlgo { Auto, Scalar };
    enum class SubAlgo { Auto, Scalar };
    enum class MulAlgo { Auto, Schoolbook, SIMD, Karatsuba };
    enum class DivAlgo { Auto, Schoolbook, Knuth };

    struct AlgoPolicy
    {
        AddAlgo add = AddAlgo::Auto;
        SubAlgo sub = SubAlgo::Auto;
        MulAlgo mul = MulAlgo::Auto;
        DivAlgo div = DivAlgo::Auto;
        size_t simdMulThresh = 16;    // mul limb 数 >= 该值时优先用 SIMD
        size_t karatsubaThresh = 64;  // mul limb 数 >= 该值时用 Karatsuba
    };

    /// 全局算法策略（可读写以强制指定算法）
    AlgoPolicy& Algo();

    //===============================================================
    // 大整数
    //===============================================================
    class BigInt
    {
        public:
            enum class State { Normal, Inf, NegInf, Nan };

            std::vector<limb> limbs;   // base=2^32，小端，已裁剪高位零（绝对值）
            bool isneg = false;
            State state = State::Normal;

            bool IsInf()    const { return state == State::Inf  || state == State::NegInf; }
            bool IsNan()    const { return state == State::Nan; }
            bool IsNormal() const { return state == State::Normal; }

            std::string type() const;
            std::string ToStr() const;

            BigInt() = default;
            BigInt(const BigInt&) = default;
            BigInt& operator=(const BigInt&) = default;
            BigInt(const std::string& str);
            explicit BigInt(State s);
            explicit BigInt(long long v);
            explicit BigInt(const char* s): BigInt(std::string(s)) {}
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
            BigInt operator+(const T& n) const { return *this + BigInt(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigInt operator-(const T& n) const { return *this - BigInt(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigInt operator*(const T& n) const { return *this * BigInt(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigInt operator/(const T& n) const { return *this / BigInt(n); }
            template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
            BigInt operator%(const T& n) const { return *this % BigInt(n); }

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

            /// 指定算法的执行入口（覆盖全局策略）
            BigInt Add(const BigInt& b, AddAlgo a) const;
            BigInt Mul(const BigInt& b, MulAlgo a) const;
            BigInt Div(const BigInt& b, DivAlgo a) const;

        private:
            template<typename T> static BigInt FromArith(T v)
            {
                if constexpr (std::is_floating_point_v<T>)
                    return BigInt(std::to_string(static_cast<long long>(v)));
                else
                    return BigInt(std::to_string(v));
            }
    };
}

    //===============================================================
    // 内部 limb 原语 base=2^32
    //===============================================================
    namespace
    {
        constexpr size_t DIGIT_CHUNK = 9;              // 十进制解析分组位数（组值 < 2^32）
        constexpr dlimb  DEC_BASE    = 1000000000ULL;   // 1e9

        inline void trim(std::vector<limb>& v){ while(v.size() > 1 && v.back() == 0) v.pop_back(); }
        inline bool isZeroMag(const std::vector<limb>& v){ return v.size() == 1 && v[0] == 0; }

        inline int magCmp(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            if(a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
            for(sdlimb i = static_cast<sdlimb>(a.size()) - 1; i >= 0; --i)
                if(a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
            return 0;
        }

        std::vector<limb> addMag(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            const size_t n = (std::max)(a.size(), b.size());
            std::vector<limb> r(n + 1, 0);
            dlimb carry = 0;
            for(size_t i = 0; i < n; ++i)
            {
                const dlimb cur = (dlimb)(i < a.size() ? a[i] : 0)
                                + (dlimb)(i < b.size() ? b[i] : 0) + carry;
                r[i] = static_cast<limb>(cur & 0xFFFFFFFFull);
                carry = cur >> 32;
            }
            r[n] = static_cast<limb>(carry);
            trim(r);
            return r;
        }

        // a >= b 的减法（绝对值）
        std::vector<limb> subMag(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            std::vector<limb> r(a.size(), 0);
            slimb borrow = 0;
            for(size_t i = 0; i < a.size(); ++i)
            {
                sdlimb d = (sdlimb)a[i] - (i < b.size() ? (sdlimb)b[i] : 0) - borrow;
                if(d < 0){ r[i] = static_cast<limb>(d + 0x100000000LL); borrow = 1; }
                else     { r[i] = static_cast<limb>(d); borrow = 0; }
            }
            trim(r);
            return r;
        }

        // r = r * c + add（c <= 1e9，add < 1e9，base=2^32）
        void mulAddSmall(std::vector<limb>& r, dlimb c, dlimb add)
        {
            dlimb carry = add;
            for(size_t i = 0; i < r.size(); ++i)
            {
                const dlimb cur = (dlimb)r[i] * c + carry;
                r[i] = static_cast<limb>(cur & 0xFFFFFFFFull);
                carry = cur >> 32;
            }
            while(carry){ r.push_back(static_cast<limb>(carry & 0xFFFFFFFFull)); carry >>= 32; }
        }

        std::vector<limb> mulSmallMag(const std::vector<limb>& a, limb c)
        {
            std::vector<limb> r(a.size() + 1, 0);
            dlimb carry = 0;
            for(size_t i = 0; i < a.size(); ++i)
            {
                const dlimb cur = (dlimb)a[i] * c + carry;
                r[i] = static_cast<limb>(cur & 0xFFFFFFFFull);
                carry = cur >> 32;
            }
            r[a.size()] = static_cast<limb>(carry);
            trim(r);
            return r;
        }

        std::vector<limb> mulMagSchool(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            std::vector<limb> r(a.size() + b.size(), 0);
            for(size_t i = 0; i < a.size(); ++i)
            {
                dlimb carry = 0;
                const limb av = a[i];
                for(size_t j = 0; j < b.size(); ++j)
                {
                    const dlimb cur = (dlimb)av * b[j] + r[i + j] + carry;
                    r[i + j] = static_cast<limb>(cur & 0xFFFFFFFFull);
                    carry = cur >> 32;
                }
                size_t k = i + b.size();
                while(carry){ const dlimb cur = (dlimb)r[k] + carry; r[k] = static_cast<limb>(cur & 0xFFFFFFFFull); carry = cur >> 32; ++k; }
            }
            trim(r);
            return r;
        }

        // 用 AVX2 逐 8-lane 计算 b[j]×av 的 64 位积（32×32→64，无进位、各 lane 独立）
        void mulRowFillSimd(std::vector<dlimb>& t, const std::vector<limb>& b, limb av)
        {
            const size_t n = b.size();
#ifdef __AVX2__
            const __m256i AV = _mm256_set1_epi32((int)av);
            size_t i = 0;
            for(; i + 8 <= n; i += 8)
            {
                __m256i B = _mm256_loadu_si256((const __m256i*)(b.data() + i));
                __m256i pe = _mm256_mul_epu32(B, AV);                 // 偶 lane：b[i+0,2,4,6]×av
                __m256i Bo = _mm256_srli_epi64(B, 32);                 // 奇 lane 挪到偶位
                __m256i po = _mm256_mul_epu32(Bo, AV);                // 奇 lane：b[i+1,3,5,7]×av
                t[i + 0] = (dlimb)_mm256_extract_epi64(pe, 0);
                t[i + 1] = (dlimb)_mm256_extract_epi64(po, 0);
                t[i + 2] = (dlimb)_mm256_extract_epi64(pe, 1);
                t[i + 3] = (dlimb)_mm256_extract_epi64(po, 1);
                t[i + 4] = (dlimb)_mm256_extract_epi64(pe, 2);
                t[i + 5] = (dlimb)_mm256_extract_epi64(po, 2);
                t[i + 6] = (dlimb)_mm256_extract_epi64(pe, 3);
                t[i + 7] = (dlimb)_mm256_extract_epi64(po, 3);
            }
            for(; i < n; ++i) t[i] = (dlimb)b[i] * av;
#else
            for(size_t i = 0; i < n; ++i) t[i] = (dlimb)b[i] * av;
#endif
        }

        // 把 t（n 个 64 位部分积）自 off 位带进位并入 r（r 保持 base=2^32）
        void addRowInto(std::vector<limb>& r, size_t off, const std::vector<dlimb>& t, size_t n)
        {
            dlimb carry = 0;
            for(size_t j = 0; j < n; ++j)
            {
                const dlimb cur = (dlimb)r[off + j] + t[j] + carry;
                r[off + j] = static_cast<limb>(cur & 0xFFFFFFFFull);
                carry = cur >> 32;
            }
            size_t k = off + n;
            while(carry)
            {
                if(k >= r.size()) r.push_back(0);
                const dlimb cur = (dlimb)r[k] + carry;
                r[k] = static_cast<limb>(cur & 0xFFFFFFFFull);
                carry = cur >> 32;
                ++k;
            }
        }

        // SIMD 乘法：乘法用 AVX2 向量化，进位传播仍为标量（乘重进轻）
        std::vector<limb> mulMagSIMD(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            const std::vector<limb>* outer = &a;
            const std::vector<limb>* inner = &b;
            if(a.size() > b.size()) std::swap(outer, inner);   // 外层较短、内层行较长 → 向量化收益大
            const size_t m = outer->size(), n = inner->size();
            std::vector<limb> r(m + n, 0);
            for(size_t i = 0; i < m; ++i)
            {
                const limb av = (*outer)[i];
                if(av == 0) continue;
                std::vector<dlimb> t(n, 0);
                mulRowFillSimd(t, *inner, av);
                addRowInto(r, i, t, n);
            }
            trim(r);
            return r;
        }

        // Karatsuba（递归），a.size() >= b.size()
        std::vector<limb> mulKaratsuba(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            const size_t n = (std::max)(a.size(), b.size());
            if(n < 24) return mulMagSchool(a, b);
            const size_t m = n / 2;
            std::vector<limb> a0(std::min(m, a.size()));
            std::vector<limb> a1(a.size() > m ? a.size() - m : 0);
            for(size_t i = 0; i < a0.size(); ++i) a0[i] = a[i];
            for(size_t i = 0; i < a1.size(); ++i) a1[i] = a[m + i];
            std::vector<limb> b0(std::min(m, b.size()));
            std::vector<limb> b1(b.size() > m ? b.size() - m : 0);
            for(size_t i = 0; i < b0.size(); ++i) b0[i] = b[i];
            for(size_t i = 0; i < b1.size(); ++i) b1[i] = b[m + i];

            trim(a0); trim(a1); trim(b0); trim(b1);
            std::vector<limb> z0 = mulKaratsuba(a0, b0);
            std::vector<limb> z2 = mulKaratsuba(a1, b1);
            std::vector<limb> sA = addMag(a0, a1);
            std::vector<limb> sB = addMag(b0, b1);
            std::vector<limb> z1 = mulKaratsuba(sA, sB);
            z1 = subMag(z1, z0);           // 需 z1 >= z0+z2……
            z1 = subMag(z1, z2);           // 若下溢则补借位保护（此处 z1 必 >= z0+z2，因加法无进位丢失）

            std::vector<limb> res(a.size() + b.size(), 0);
            const size_t SH = m;
            auto addAt = [&](const std::vector<limb>& v, size_t off){
                dlimb carry = 0;
                for(size_t i = 0; i < v.size(); ++i)
                {
                    const dlimb cur = (dlimb)(off + i < res.size() ? res[off + i] : 0) + v[i] + carry;
                    if(off + i >= res.size()) res.resize(off + i + 1, 0);
                    res[off + i] = static_cast<limb>(cur & 0xFFFFFFFFull);
                    carry = cur >> 32;
                }
                size_t k = off + v.size();
                while(carry){ if(k >= res.size()) res.push_back(0); const dlimb cur = (dlimb)res[k] + carry; res[k] = static_cast<limb>(cur & 0xFFFFFFFFull); carry = cur >> 32; ++k; }
            };
            addAt(z0, 0);
            addAt(z1, SH);
            addAt(z2, 2 * SH);
            trim(res);
            return res;
        }

        struct DivResult { std::vector<limb> q, r; };

        DivResult divMagKnuth(const std::vector<limb>& a, const std::vector<limb>& b)
        {
            if(isZeroMag(b)) return {{limb(0)}, std::vector<limb>{0}};
            // 单 limb 除数
            if(b.size() == 1)
            {
                std::vector<limb> q = a;
                const dlimb d = b[0];
                dlimb remi = 0;
                for(sdlimb i = static_cast<sdlimb>(q.size()) - 1; i >= 0; --i)
                {
                    const dlimb cur = (remi << 32) | q[i];
                    q[i] = static_cast<limb>(cur / d);
                    remi = cur % d;
                }
                trim(q);
                return {q, std::vector<limb>{static_cast<limb>(remi)}};
            }
            if(magCmp(a, b) < 0) return {{limb(0)}, a};
            std::vector<limb> q(a.size() - b.size() + 1, 0);
            std::vector<limb> remv{0};
            for(sdlimb i = static_cast<sdlimb>(a.size()) - 1; i >= 0; --i)
            {
                remv.insert(remv.begin(), a[i]);
                trim(remv);
                const size_t qi = static_cast<size_t>(i);
                if(qi < q.size() && magCmp(remv, b) >= 0)
                {
                    dlimb lo = 0, hi = 0x100000000ULL; // hi 独占
                    while(lo + 1 < hi)
                    {
                        const dlimb mid = (lo + hi) >> 1;
                        std::vector<limb> t = mulSmallMag(b, static_cast<limb>(mid));
                        trim(t);
                        if(magCmp(t, remv) <= 0) lo = mid; else hi = mid;
                    }
                    q[qi] = static_cast<limb>(lo);
                    if(lo)
                    {
                        std::vector<limb> t = mulSmallMag(b, static_cast<limb>(lo));
                        trim(t);
                        remv = subMag(remv, t);
                    }
                }
            }
            trim(q);
            trim(remv);
            return {q, remv};
        }

        // 需 z1 >= z0 + z2 的保护处理封装（Karatsuba 用）
        std::vector<limb> subMagWithBorrowOK(const std::vector<limb>& a, const std::vector<limb>& b)
        { return subMag(a, b); }

        // 十进制字符串 -> base=2^32 limbs（不含符号符）
        // 从最高位起按 9 位分组，逐组做 r = r * 1e9 + chunk
        std::vector<limb> fromDecDigits(const std::string& digits)
        {
            std::vector<limb> r{0};
            const size_t n = digits.size();
            if(n == 0) return r;
            size_t take = n % DIGIT_CHUNK;
            if(take == 0) take = DIGIT_CHUNK;
            size_t p = 0;
            while(p < n)
            {
                const uint64_t chunk = strtoull(digits.substr(p, take).c_str(), nullptr, 10);
                mulAddSmall(r, DEC_BASE, chunk);
                p += take;
                take = DIGIT_CHUNK;
            }
            trim(r);
            return r;
        }

        // base=2^32 limbs -> 十进制字符串（绝对值）
        std::string toDecDigits(const std::vector<limb>& mag)
        {
            if(isZeroMag(mag)) return "0";
            std::vector<limb> v = mag;
            std::vector<std::string> chunks;
            while(!(v.size() == 1 && v[0] == 0))
            {
                dlimb remi = 0;
                for(sdlimb i = static_cast<sdlimb>(v.size()) - 1; i >= 0; --i)
                {
                    const dlimb cur = (remi << 32) | v[i];
                    v[i] = static_cast<limb>(cur / DEC_BASE);
                    remi = cur % DEC_BASE;
                }
                trim(v);
                chunks.push_back(std::to_string(remi));
            }
            std::string res = chunks.back();
            for(sdlimb i = static_cast<sdlimb>(chunks.size()) - 2; i >= 0; --i)
                res += std::string(DIGIT_CHUNK - chunks[i].size(), '0') + chunks[i];
            return res;
        }

        // 解析含 inf/nan 的字符串，返回状态
        BigInt::State tokState(std::string s)
        {
            size_t b = 0, e = s.size();
            while(b < e && isspace(static_cast<unsigned char>(s[b]))) ++b;
            while(e > b && isspace(static_cast<unsigned char>(s[e-1]))) --e;
            for(size_t i = b; i < e; ++i) s[i] = static_cast<char>(std::tolower((unsigned char)s[i]));
            const std::string t = s.substr(b, e - b);
            if(t == "inf" || t == "+inf") return BigInt::State::Inf;
            if(t == "-inf") return BigInt::State::NegInf;
            if(t == "nan")  return BigInt::State::Nan;
            return BigInt::State::Normal;
        }

        size_t sigDigitCount(const std::string& s)
        {
            size_t n = 0;
            for(char c : s) if(c >= '0' && c <= '9') ++n;
            return n;
        }

        std::random_device bn_rd;
        std::mt19937 bn_gen(bn_rd());
        sdlimb bn_RandInt(sdlimb mn, sdlimb mx){ std::uniform_int_distribution<sdlimb> d(mn, mx); return d(bn_gen); }
    }

    AlgoPolicy& Algo()
    {
        static AlgoPolicy P;
        return P;
    }

    //===============================================================
    // BigInt 实现
    //===============================================================
    static std::string NormalizeInt(const std::string& str)
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
            if(isdigit((unsigned char)c))
            {
                if(c == '0' && !numstart && !dot){ ++rp; continue; }
                numstart = true; res[wp++] = c; ++rp;
            }
            else if(c == '.' && !dot)
            {
                if(!numstart){ res.insert(res.begin() + wp, 1, '0'); ++wp; }
                dot = true; res[wp++] = '.'; ++rp;
            }
            else if(!numstart && (c == '-' || c == '+')){ if(c == '-') isneg = !isneg; ++rp; }
            else ++rp;
        }
        if(!numstart) return "0";
        res[0] = isneg ? '-' : '+';
        res.resize(wp);
        if(!res.empty() && res.back() == '.') res.pop_back();
        if(dot){ while(!res.empty() && res.back() == '0') res.pop_back(); if(!res.empty() && res.back() == '.') res.pop_back(); }
        if(res.size() <= 1) return "0";
        for(size_t i = 1; i < res.size(); ++i) if(res[i] != '0') return res;
        return "0";
    }

    BigInt::BigInt(const std::string& str)
    {
        const BigInt::State st = tokState(str);
        if(st != BigInt::State::Normal) { state = st; isneg = (st == BigInt::State::NegInf); limbs = {0}; return; }
        const std::string n = NormalizeInt(str);
        if(n == "0") { limbs = {0}; isneg = false; return; }
        isneg = (n[0] == '-');
        std::string digits;
        for(size_t i = 1; i < n.size(); ++i)
        {
            if(n[i] == '.') break;          // 整数部分：小数部分截断
            digits += n[i];
        }
        limbs = fromDecDigits(digits.empty() ? "0" : digits);
        if(isZeroMag(limbs)) isneg = false;
    }

    BigInt::BigInt(State s){ state = s; isneg = (s == State::NegInf); limbs = {0}; }
    BigInt::BigInt(long long v)
    {
        if(v == 0){ limbs = {0}; return; }
        isneg = (v < 0);
        unsigned long long a = isneg ? static_cast<unsigned long long>(-(v + 1)) + 1ULL : static_cast<unsigned long long>(v);
        limbs = fromDecDigits(std::to_string(a));
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
        if(isZeroMag(limbs)) return "0";
        std::string d = toDecDigits(limbs);
        return isneg ? "-" + d : d;
    }

    BigInt BigInt::operator-() const
    {
        BigInt r = *this;
        if(state == State::Inf) r.state = State::NegInf;
        else if(state == State::NegInf) r.state = State::Inf;
        else if(state == State::Normal && !isZeroMag(limbs)) r.isneg = !isneg;
        return r;
    }

    BigInt BigInt::Add(const BigInt& b, AddAlgo a) const
    {
        (void)a;   // 加法已统一标量实现
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if((IsInf() && b.IsInf() && state != b.state)) return BigInt(State::Nan);
            return BigInt(state != State::Normal ? state : b.state);
        }
        BigInt res; res.state = State::Normal;
        if(isneg == b.isneg)
        {
            res.limbs = addMag(limbs, b.limbs);
            res.isneg = isneg;
        }
        else
        {
            const int c = magCmp(limbs, b.limbs);
            if(c == 0) return BigInt(0);
            if(c > 0){ res.limbs = subMag(limbs, b.limbs); res.isneg = isneg; }
            else     { res.limbs = subMag(b.limbs, limbs); res.isneg = b.isneg; }
        }
        return res;
    }

    BigInt BigInt::operator+(const BigInt& b) const { return Add(b, Algo().add); }
    BigInt BigInt::operator-(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        if(state != State::Normal || b.state != State::Normal)
        {
            if(IsInf() && b.IsInf() && state == b.state) return BigInt(State::Nan);
            if(state != State::Normal) return BigInt(state);
            return BigInt(b.state == State::Inf ? State::NegInf : State::Inf);
        }
        BigInt res; res.state = State::Normal;
        if(isneg != b.isneg)
        {
            res.limbs = addMag(limbs, b.limbs);
            res.isneg = isneg;
        }
        else
        {
            const int c = magCmp(limbs, b.limbs);
            if(c == 0) return BigInt(0);
            if(c > 0){ res.limbs = subMag(limbs, b.limbs); res.isneg = isneg; }
            else     { res.limbs = subMag(b.limbs, limbs); res.isneg = !isneg; }
        }
        return res;
    }

    BigInt BigInt::Mul(const BigInt& b, MulAlgo a) const
    {
        if(state == State::Nan || b.state == State::Nan) return BigInt(State::Nan);
        const bool ai = IsInf(), bi = b.IsInf();
        if(ai || bi)
        {
            const bool az = state == State::Normal && isZeroMag(limbs);
            const bool bz = b.state == State::Normal && isZeroMag(b.limbs);
            if((ai && bz) || (bi && az)) return BigInt(State::Nan);
            const bool an = (state == State::NegInf) || (state == State::Normal && isneg);
            const bool bn = (b.state == State::NegInf) || (b.state == State::Normal && b.isneg);
            return BigInt((an ^ bn) ? State::NegInf : State::Inf);
        }
        if(a == MulAlgo::Auto)
        {
            const size_t n = (std::max)(limbs.size(), b.limbs.size());
            const AlgoPolicy& P = Algo();
            if(P.mul == MulAlgo::Karatsuba || n >= P.karatsubaThresh) a = MulAlgo::Karatsuba;
            else if(P.mul == MulAlgo::SIMD || n >= P.simdMulThresh)   a = MulAlgo::SIMD;
            else                                                      a = MulAlgo::Schoolbook;
        }
        BigInt res; res.state = State::Normal;
        if(a == MulAlgo::Karatsuba)
            res.limbs = limbs.size() >= b.limbs.size() ? mulKaratsuba(limbs, b.limbs) : mulKaratsuba(b.limbs, limbs);
        else if(a == MulAlgo::SIMD)
            res.limbs = mulMagSIMD(limbs, b.limbs);
        else
            res.limbs = mulMagSchool(limbs, b.limbs);
        res.isneg = isneg ^ b.isneg;
        if(isZeroMag(res.limbs)) res.isneg = false;
        return res;
    }

    BigInt BigInt::operator*(const BigInt& b) const { return Mul(b, Algo().mul); }

    BigInt DivMod(const BigInt& a, const BigInt& b, bool wantRem, DivAlgo algo)
    {
        if(a.state == BigInt::State::Nan || b.state == BigInt::State::Nan) return BigInt(BigInt::State::Nan);
        if(a.IsInf() || b.IsInf())
        {
            if(wantRem || (a.IsInf() && b.IsInf())) return BigInt(BigInt::State::Nan);
            if(a.IsInf())
            {
                const bool an = (a.state == BigInt::State::NegInf) || (a.state == BigInt::State::Normal && a.isneg);
                const bool bn = (b.state == BigInt::State::NegInf) || (b.state == BigInt::State::Normal && b.isneg);
                return BigInt((an ^ bn) ? BigInt::State::NegInf : BigInt::State::Inf);
            }
            return BigInt(0);
        }
        if(b.isneg ? b == BigInt("0") : isZeroMag(b.limbs))
        {
            KLOG_ERROR(KBIGNUM_DIVBYZERO, wantRem ? "module by zero" : "divide by zero");
            return isZeroMag(a.limbs) ? BigInt(BigInt::State::Nan)
                : BigInt((a.isneg ^ b.isneg) ? BigInt::State::NegInf : BigInt::State::Inf);
        }
        if(isZeroMag(a.limbs)) return BigInt(0);
        DivResult d = divMagKnuth(a.limbs, b.limbs);
        BigInt res; res.state = BigInt::State::Normal;
        res.limbs = wantRem ? d.r : d.q;
        res.isneg = wantRem ? a.isneg : (a.isneg ^ b.isneg);
        if(isZeroMag(res.limbs)) res.isneg = false;
        return res;
    }

    BigInt BigInt::Div(const BigInt& b, DivAlgo a) const { return DivMod(*this, b, false, a); }
    BigInt BigInt::operator/(const BigInt& b) const { return DivMod(*this, b, false, Algo().div); }
    BigInt BigInt::operator%(const BigInt& b) const { return DivMod(*this, b, true, DivAlgo::Knuth); }

    bool BigInt::operator==(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state != State::Normal || b.state != State::Normal) return state == b.state;
        return isneg == b.isneg && magCmp(limbs, b.limbs) == 0;
    }
    bool BigInt::operator!=(const BigInt& b) const { return !(*this == b); }
    bool BigInt::operator<(const BigInt& b) const
    {
        if(state == State::Nan || b.state == State::Nan) return false;
        if(state == State::NegInf) return b.state != State::NegInf;
        if(b.state == State::Inf) return state != State::Inf;
        if(state == State::Inf) return false;
        if(b.state == State::NegInf) return false;
        if(isneg != b.isneg) return isneg;
        const int c = magCmp(limbs, b.limbs);
        return isneg ? (c > 0) : (c < 0);
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
        const uint64_t mag = (dlimb)limbs[0] | ((limbs.size() == 2) ? (dlimb)limbs[1] << 32 : 0);
        return mag <= 9223372036854775807ULL;
    }
    bool BigInt::IsInDoubleRange() const { return sigDigitCount(ToStr()) <= 308; }

    //===============================================================
    // 自由函数（BigInt 相关）
    //===============================================================
    export std::string Normalize(const std::string& str){ return NormalizeInt(str); }

    export BigInt BigGcd(BigInt a, BigInt b)
    {
        a.isneg = b.isneg = false;
        a.state = b.state = BigInt::State::Normal;
        while(b != BigInt(0))
        {
            BigInt r = a % b;
            a = b; b = r;
        }
        return a;
    }

    export BigInt RandBigInt(std::pair<size_t,size_t> IntRand = {0,0}, int sign = 0)
    {
        if(IntRand.second < IntRand.first) std::swap(IntRand.first, IntRand.second);
        bool isneg;
        if(sign == 1) isneg = false;
        else if(sign == 2) isneg = true;
        else isneg = (bn_RandInt(0, 1) == 1);
        const size_t ns = IntRand.second == 0 ? 0 : (size_t)bn_RandInt((sdlimb)IntRand.first, (sdlimb)IntRand.second);
        if(ns == 0) return BigInt(0);
        BigInt res;
        res.limbs.clear();
        res.isneg = isneg;
        res.state = BigInt::State::Normal;
        for(size_t i = 0; i < ns; ++i)
            res.limbs.push_back((limb)bn_RandInt(0, 0x7FFFFFFFl));
        if(res.limbs.back() == 0) res.limbs.back() = 1;
        res.limbs.push_back(1);       // 保证高位非零（可控低位上限）
        trim(res.limbs);
        if(isZeroMag(res.limbs)) return BigInt(0);
        if(isneg) res.isneg = true;
        return res;
    }