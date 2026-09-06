#include "modules/cpp/KF.hpp"
using namespace std;
using namespace KMATH;
using namespace KCLI;
using namespace KSON;
using namespace KTIMER;

size_t n;
vector<BigDec> arr;
long long printSleep = 0; // 每次打印后的停顿(ms)，来自 GLOBAL["ArrPrintSleep"]
size_t printEvery = 1;   // 每打印一次间隔的比较数（用户输入），0=不停顿

const int barMax = 50;

/// @brief 计算每个位置的序号（1=最小，n=最大）
vector<size_t> ComputeRanks(const vector<BigDec>& a)
{
    vector<size_t> ranks(n);
    for (size_t i = 0; i < n; i++)
    {
        ranks[i] = 1;
        for (size_t j = 0; j < n; j++)
            if (a[j] < a[i]) ranks[i]++;
    }
    return ranks;
}

/// @brief 判断数组是否已按 rule 有序（1=升序，2=降序）
static bool IsSorted(const vector<BigDec>& a, size_t rule)
{
    for (size_t i = 1; i < n; i++)
    {
        if ((rule == 1 && a[i - 1] > a[i]) || (rule == 2 && a[i - 1] < a[i]))
            return false;
    }
    return true;
}

/// @brief 随机打乱数组（猴子排序的核心）
static void Shuffle(vector<BigDec>& a)
{
    static std::mt19937 g{std::random_device{}()};
    std::shuffle(a.begin(), a.end(), g);
}

void BogoSort(vector<BigDec>& a, size_t rule)
{
    AddTimer("BOGO SORT", TimeUnit::us);
    if (printSleep > 0) { ClearScreen(); }

    size_t cnt = 0;
    vector<size_t> ranks = ComputeRanks(a);
    while (!IsSorted(a, rule))
    {
        Shuffle(a);
        ranks = ComputeRanks(a); // 洗牌后名次跟随变化
        ++cnt;
        if (printSleep > 0 && cnt % printEvery == 0)
        {
            Arr::Print(ranks, n, barMax);
            Sleep((DWORD)printSleep);
        }
    }

    PrintTimer("BOGO SORT");
    if (printSleep > 0) { Arr::Print(ranks, n, barMax); }
    kout << "{green}\nSort complete!{/}" << endl;

    bool out = true;
    kout << "Output result?: ";
    kin >> out;
    ClearScreen();
    if (out)
    {
        kout << "result :\n";
        for (auto x : a)
            koutW << x << "\n";
        kout << endl;
    }
}

int main()
{
    kson file = ReadKsonFile("config/algorithm/cfg.kson");
    kson main = file["Algorithm"]["Sorting"]["BogoSort"];
    KBegin(main);
    size_t rule = KOptions(main["sort_method"]);
    bool isgen = false;
    kout << "size of array: ";
    kin >> n;
    kout << "Automate generate BIGNUM? {yellow}(Bool):{/}";
    kin >> isgen;
    if (isgen)
    {
        size_t iMin, iMax, dMin, dMax;
        int sign;
        kout << "integer digit range (min max): ";
        kin >> iMin >> iMax;
        kout << "decimal digit range (min max): ";
        kin >> dMin >> dMax;
        kout << "sign (0=random 1=positive 2=negative): ";
        kin >> sign;
        for (size_t i = 0; i < n; i++)
            arr.push_back(RandBigDec({iMin, iMax}, {dMin, dMax}, sign));
    }
    else
    {
        kout << "input array: \n";
        for (size_t i = 0; i < n; i++)
        {
            BigDec x;
            kin >> x;
            arr.push_back(x);
        }
    }
    printSleep = GLOBAL["ArrPrintSleep"].Int();
    kout << "Pause every N shuffles (0=off, recommended 1): ";
    kin >> printEvery;
    if (printSleep > 0) CheckConsoleFit((int)n + 3, barMax + 3);
    BogoSort(arr, rule);
    kout << "Time Complexity: " << main["time_complexity"].Str() << "\n\n";
    KEnd();
}