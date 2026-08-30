#include "linked_list.hpp"
using namespace KMATH;
using namespace KCLI;
using namespace KSON;
using namespace KTIMER;
using namespace std;
using namespace KLIST::SINGLY;
int main()
{
    kson main = ReadKsonFile("config/algorithm/cfg.kson")["Algorithm"]["graph"]["linked_list"]["singly"];
    KBegin(main);
    long long DefaultSize = main["default_size"].Int();
    long long DefaultDataLen = main["default_data_len"].Int();

    List<BigDec> list;

    kout << "{skyblue}=== 单向链表演示 ==={/}\n\n";
    kout << "初始状态: size=" << list.size()
         << "  empty=" << (list.empty() ? "true" : "false") << "\n\n";

    // ── push_back 批量尾插 ──
    kout << "{yellow}[1] push_back{/}  尾插 " << DefaultSize << " 个随机大数 (绿色=新插入):\n  ";
    AddTimer("PUSH_BACK", TimeUnit::us);
    for (int i = 0; i < DefaultSize; i++)
        list.push_back(RandBigDec({static_cast<size_t>(DefaultDataLen), static_cast<size_t>(DefaultDataLen)}, {0, 0}, 1));
    PauseTimer("PUSH_BACK");
    list.print(list.size() - 1, Color::Green);
    kout << "  size=" << list.size() << "\n";
    PrintTimer("PUSH_BACK");

    // ── at 随机访问 ──
    kout << "\n{yellow}[2] at(index){/}  随机访问:\n";
    kout << "  at(0)             = " << list.at(0) << "\n";
    kout << "  at(" << list.size() / 2 << ")             = " << list.at(list.size() / 2) << "\n";
    kout << "  at(" << list.size() - 1 << ")            = " << list.at(list.size() - 1) << "\n";

    // ── find 查找 ──
    BigDec target = list.at(list.size() / 2);
    kout << "\n{yellow}[3] find(target){/}  查找元素 " << target << "\n";
    AddTimer("FIND", TimeUnit::us);
    int64_t pos = list.find(target);
    PauseTimer("FIND");
    if (pos == -1) kout << "  {red}未找到{/}\n";
    else           kout << "  {green}找到于下标 " << pos << "{/}\n";
    PrintTimer("FIND");

    BigDec notexist("999999999999");
    kout << "  查找不存在的 " << notexist << ": ";
    pos = list.find(notexist);
    if (pos == -1) kout << "{red}未找到 (返回 -1){/}\n";
    else           kout << "找到于下标 " << pos << "\n";

    // ── push_front 头插 ──
    BigDec frontval("123456789");
    kout << "\n{yellow}[4] push_front{/}  头插 " << frontval << " (绿色=新插入):\n  ";
    list.push_front(frontval);
    list.print(0, Color::Green);
    kout << "  size=" << list.size() << "\n";

    // ── insert 中间插入 ──
    size_t mid = list.size() / 2;
    BigDec midval("77777777");
    kout << "\n{yellow}[5] insert(data, " << mid << "){/}  中间插入 " << midval << " (绿色=新插入):\n  ";
    list.insert(midval, mid);
    list.print(mid, Color::Green);
    kout << "  size=" << list.size() << "\n";

    // ── erase 删除 ──
    kout << "\n{yellow}[6] erase_front / erase_back / erase(mid){/}  (红色=待删除):\n";

    kout << "  erase_front  删头元素:\n  ";
    list.print(0, Color::Red);
    list.erase_front();
    kout << "  → size=" << list.size() << "\n";

    kout << "  erase_back   删尾元素:\n  ";
    list.print(list.size() - 1, Color::Red);
    list.erase_back();
    kout << "  → size=" << list.size() << "\n";

    size_t emid = list.size() / 2;
    kout << "  erase(" << emid << ")     删中间元素:\n  ";
    list.print(emid, Color::Red);
    list.erase(emid);
    kout << "  → size=" << list.size() << "\n  ";
    list.print();

    // ── at(index) = value 修改测试 ──
    size_t upIdx = list.size() / 2;
    BigDec oldval = list.at(upIdx);
    BigDec newval("88888888");
    kout << "\n{yellow}[7] at(" << upIdx << ") = " << newval << "{/}  修改元素 (绿色=已修改):\n";
    kout << "  旧值: " << oldval << "  →  新值: " << newval << "\n  ";
    
    list.at(upIdx) = newval;
    list.print(upIdx, Color::Green);

    // ── clear 清空 ──
    kout << "\n{yellow}[8] clear(){/}  清空链表:\n";
    list.clear();
    kout << "  size=" << list.size()
         << "  empty=" << (list.empty() ? "true" : "false") << "\n";

    // ── 时间复杂度 ──
    kout << "\n{skyblue}=== 时间复杂度 ==={/}\n";
    kout << "  search: " << main["time_complexity"]["search"].Str() << "\n";
    kout << "  insert: " << main["time_complexity"]["insert"].Str() << "\n";
    kout << "  delete: " << main["time_complexity"]["delete"].Str() << "\n";

    KEnd();
}
