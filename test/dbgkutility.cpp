#include "base/KF.hpp"
using namespace KMATH;
int main()
{
    KCLI::KBegin("KForge Utility", "Random big number generator","Git-1145","2026-08-13");
    // 整数位数范围 [1,8]，小数位数范围 [0,5]，符号随机(0)
    for(size_t i = 0 ; i < 10 ; i++)
        KCLI::kout << RandBigNum({1,8},{0,5},0) << "\n";
    KCLI::KEnd();
}
