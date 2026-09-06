#include "modules/cpp/KF.hpp"
using namespace std;
using namespace KFIO;
using namespace KSON;
using namespace KCLI;
using namespace KTIMER;
using namespace KF;

size_t totalVisited = 0;      // 已探索格子数
size_t totalCells = 0;        // 迷宫 P 总数
size_t printEvery = 0;        // 每打印一次间隔的步数（用户输入），0=不打印
static long long printSleep;  // 每次打印后的停顿(ms)，来自 GLOBAL["MazePrintSleep"]
bool foundExit = false;
vector<vector<MazeCell>> maze;
int rows, cols;

void DFS(int startR, int startC, int endR, int endC)
{
    /// @attention 此函数 使用显式栈 实现 DFS 不是递归
    stack<pair<int,int>> st;
    st.push({startR, startC});
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    while (!st.empty() && !foundExit)
    {
        auto [r, c] = st.top(); st.pop();

        for (int i = 0; i < 4; i++)
        {
            int nr = r + dr[i], nc = c + dc[i];
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;

            MazeCell& cell = maze[nr][nc];
            if (cell == WALL || cell == VISITED || cell == START) continue;
            if (cell == END) { foundExit = true; break; }

            cell = VISITED;
            totalVisited++;
            st.push({nr, nc});

            if (printEvery > 0 && totalVisited % printEvery == 0)
            {
                Maze::Print(maze);
                kout << "Visited:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
                Sleep(printSleep);
            }
        }
    }
}

int main()
{
    kson file = ReadKsonFile("config/algorithm/cfg.kson");
    KBegin(file["Algorithm"]["Searching"]["DFS"]);
    printSleep = GLOBAL["MazePrintSleep"].Int();
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    kson doc = ReadKsonFile("config/algorithm/maze.kson");
    const Node* mazeNode = doc["maze"].Resolve();
    const auto& mazes = mazeNode->AsObj();

    vector<string> names;
    names.reserve(mazes.size());

    kout << "Available mazes:" << endl;
    for (const auto& [key, _] : mazes)
    {
        names.push_back(key);
        kout << "  - " << key << endl;
    }

    string choice;
    kout << "Enter maze name (default: small): ";
    kin >> choice;
    if (choice.empty()) choice = "small";

    bool found = false;
    for (auto& name : names)
        if (name == choice) { found = true; break; }

    if (!found)
    {
        kout << "{lightyellow}Maze '" << choice << "' not found, using 'small'{/}" << endl;
        choice = "small";
    }
    maze = ReadMaze("config/algorithm/maze.kson", choice);
    int rows = (int)maze.size();
    int cols = (int)maze[0].size();

    totalCells = 0;
    for (const auto& row : maze)
        for (auto cell : row)
            if (cell == PASSABLE)
                totalCells++;

    int startR = -1, startC = -1, endR = -1, endC = -1;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
        {
            if (maze[r][c] == START) { startR = r; startC = c; }
            if (maze[r][c] == END)   { endR = r; endC = c; }
        }

    kout << "Solving maze (" << rows << "x" << cols << ")..." << endl;
    kout << "Start: (" << startR << "," << startC << ")" << endl;
    kout << "End:   (" << endR << "," << endC << ")" << endl;

    kout << "Print pause (ms, 0=no print): ";
    kin >> printInterval;

    AddTimer("search", TimeUnit::us);
    DFS(startR, startC, endR, endC);
    PauseTimer("search");

    system("cls");
    PrintMaze(maze);
    kout << endl;
    kout << "{bold}Search complete!{/}" << endl;
    kout << "Visited cells:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
    PrintTimer("search");
    if (foundExit)
        kout << "{green}Exit found!{/}" << endl;
    else
        kout << "{red}No path to exit!{/}" << endl;
    KEnd();
}