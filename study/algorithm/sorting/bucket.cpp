#include "base/KF.hpp"
using namespace std;
using namespace KBIGNUM;
using namespace KCLI;
using namespace KUTIL;
using namespace KSON;
using namespace KTIMER;
using NUM = uint32_t;
NUM n;
vector<NUM> arr;
long long printSleep = 0; // 每次打印后的停顿(ms)，来自 GLOBAL["ArrPrintSleep"]
size_t printEvery = 1;   // 每打印一次间隔的写入数（用户输入），0=不停顿
long long BarMax = 50;
void BucketSort(vector<NUM>& a, size_t rule)
{
    if (a.empty()) return;
    const size_t n = a.size();
    NUM maxnum = *max_element(a.begin(), a.end());
    vector<size_t> tmp(maxnum + 1, 0);
    vector<NUM> res;
    res.reserve(n);
    AddTimer("BUCKET SORT",TimeUnit::us);

    if (printSleep > 0) ClearScreen();
    vector<size_t> bar(n, 0);   // 各位置名次（1..n），桶序天然有序 → 名次=当前位置
    size_t filled = 0;
    size_t cnt = 0;
    auto put = [&](NUM v) {
        res.push_back(v);
        bar[filled] = filled + 1;           // 桶序从小到大填，当前位置即最终有序位
        filled++;
        ++cnt;
        if (printSleep > 0 && cnt % printEvery == 0)
        {
            Arr::Print(bar, n, BarMax, (int)(filled - 1));
            Sleep((DWORD)printSleep);
        }
    };

    // 计数
    for(size_t i = 0; i < n; i++)
        tmp[a[i]]++;
    if(rule == 1) // ascending
    {
        for(size_t i = 0; i < tmp.size(); i++)
            if(tmp[i] != 0)
                for(size_t j = 0; j < tmp[i]; j++)
                    put(static_cast<NUM>(i));
    }
    else // descending
    {
        for(size_t i = tmp.size()-1; i > 0; i--)
            if(tmp[i] != 0)
                for(size_t j = 0; j < tmp[i]; j++)
                    put(static_cast<NUM>(i));
    }
    PrintTimer("BUCKET SORT");
    if (printSleep > 0) { Arr::Print(bar, n, BarMax); }
    bool out = true;
    kout << "Output result?: ";
    kin >> out;
    if(out)
    {
        kout << "result :\n";
        for(size_t i = 0; i < res.size(); i++)
            kout << res[i] << endl;
        kout << endl;
    }
}
int main()
{
    kson file = ReadKsonFile("config/algorithm/cfg.kson");
    kson main = file["Algorithm"]["Sorting"]["BucketSort"];
    KBegin(main);
    printSleep = GLOBAL["ArrPrintSleep"].Int();
    BarMax = GLOBAL["BarMax"].Int();
    size_t rule = KOptions(main["sort_method"]);
    bool isgen=false;
    kout << "size of array: ";
    kin >> n;
    arr.clear();
    try
    {
        arr.reserve(n); // 预分配容量
    } catch (const std::bad_alloc& e) {
        Fatal(SYSTEM_OOM, "can not reserve the vector",__FILE__,__LINE__,__FUNCTION__);
    }
    kout << "Automate generate INTEGER? {yellow}(Bool):{/}";
    kin >> isgen;
    if(isgen)
    {
        NUM iMin, iMax;
        kout << "integer digit range (min max): ";
        kin >> iMin >> iMax;
        for(size_t i = 0; i < n; i++)
            arr.push_back(RandInt(iMin, iMax));
    }
    else
    {
        kout << "input array: \n";
        for(size_t i = 0; i < n; i++)
        {
            NUM x;
            kin >> x;
            arr.push_back(x);
        }
    }
    kout << "Pause every N writes (0=off, recommended 1): ";
    kin >> printEvery;
    if (printSleep > 0) CheckConsoleFit((int)n + 3, BarMax + 3);
    BucketSort(arr, rule);
    kout << "Time Complexity: " << main["time_complexity"].Str() << "\n\n";
    KEnd();
}