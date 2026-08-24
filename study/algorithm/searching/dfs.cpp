#include "base/KF.hpp"
#include <thread>
#include <chrono>

using namespace std;
using namespace KFIO;
using namespace KSON;
using namespace KCLI;
using namespace KTIMER;
using namespace KF;

int totalVisited = 0;      // 已探索格子数
int totalCells = 0;        // 迷宫 P 总数
bool foundExit = false;
vector<vector<MazeCell>> maze;
int printInterval = 0;     // 打印停顿(ms)，0=静默

// r=行号, c=列号
void DFS(int r, int c, int rows, int cols)
{
    if (foundExit) return;

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++)
    {
        int nr = r + dr[i], nc = c + dc[i];
        if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;

        MazeCell& cell = maze[nr][nc];
        if (cell == END) { foundExit = true; return; }
        if (cell == WALL || cell == VISITED || cell == START) continue;

        cell = VISITED;
        totalVisited++;
        DFS(nr, nc, rows, cols);
        if (foundExit) return;

        if (printInterval > 0)
        {
            PauseTimer("search");
            system("cls");
            PrintMaze(maze);
            kout << "Visited:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
            this_thread::sleep_for(chrono::milliseconds(printInterval));
            StartTimer("search");
        }
    }
}

int main()
{
    kson file = ReadKsonFile("config/algorithm/cfg.kson");
    KBegin(file["Algorithm"]["Searching"]["DFS"]);

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
    /// @brief 提前看迷宫 获得基本信息
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
    /// @brief 开始搜索
    AddTimer("search", TimeUnit::us);
    DFS(startR, startC, rows, cols);
    PauseTimer("search");
    /// @brief 打印结果
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

    KPause();
    return 0;
}