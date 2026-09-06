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

long long BarMax = 50;

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

void BubbleSort(vector<BigDec>& a, vector<size_t> ranks, size_t rule)
{
    AddTimer("BUBBLE SORT", TimeUnit::us);

    if (printSleep > 0) { ClearScreen(); Arr::Print(ranks, n, BarMax); }

    size_t cnt = 0;
    for (size_t i = 0; i < n - 1; i++)
    {
        for (size_t j = 0; j < n - i - 1; j++)
        {
            ++cnt;
            if (printSleep > 0 && cnt % printEvery == 0)
            {
                Arr::Print(ranks, n, BarMax, (int)j, (int)(j + 1), (int)(n - i - 1));
                Sleep((DWORD)printSleep);
            }

            if ((rule == 1 && a[j] > a[j + 1]) || (rule == 2 && a[j] < a[j + 1]))
            {
                swap(a[j], a[j + 1]);
                swap(ranks[j], ranks[j + 1]);
            }
        }
    }

    PrintTimer("BUBBLE SORT");
    if (printSleep > 0) { Arr::Print(ranks, n, BarMax); }
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
    kson main = file["Algorithm"]["Sorting"]["BubbleSort"];
    KBegin(main);
    size_t rule = KOptions(main["sort_method"]);
    bool isgen = false;
    BarMax = GLOBAL["BarMax"].Int();

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
    kout << "Pause every N compares (0=off, recommended 1): ";
    kin >> printEvery;
    if (printSleep > 0) CheckConsoleFit((int)n + 3, BarMax + 3);
    auto ranks = ComputeRanks(arr);
    BubbleSort(arr, ComputeRanks(arr), rule);
    kout << "Time Complexity: " << main["time_complexity"].Str() << "\n\n";
    KEnd();
}