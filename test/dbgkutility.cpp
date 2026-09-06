import kf;
import kbignum;

int main()
{
    // KBegin({cmdtitle(=title), description, author, date})
    KBegin({"KForge Utility", "Random big number generator", "Git-1145", "2026-08-13"});
    // 整数位数范围 [1,8]，小数位数范围 [0,5]，符号随机(0)
    for(size_t i = 0 ; i < 10 ; i++)
        kout << RandBigNum({1,8},{0,5},0) << "\n";
    KEnd();
}