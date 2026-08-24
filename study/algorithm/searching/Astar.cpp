#include "base/KF.hpp"
using namespace std;
using namespace KFIO;
using namespace KSON;
using namespace KCLI;
using namespace KF;
using namespace KTIMER;
size_t totalVisited = 0;      // 已探索格子数
size_t totalCells = 0;        // 迷宫 P 总数
size_t printEvery = 0;        // 每打印一次间隔的步数（用户输入），0=不打印
static long long printSleep;        // 每次打印后的停顿(ms)，来自 GLOBAL["PRINTSLEEP"]
bool foundExit = false;
vector<vector<MazeCell>> maze;
int rows, cols;
int startX = -1, startY = -1, endX = -1, endY = -1;
size_t pathlen = 0;           // 最短路径长度

void AStar()
{
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    // g 值（从起点到当前格子的实际代价）
    vector<vector<int>> g(rows, vector<int>(cols, INT_MAX));
    // 父节点（用于回溯路径）
    vector<vector<pair<int,int>>> Pre(rows, vector<pair<int,int>>(cols, {-1, -1}));

    // 优先队列：按 f = g + h 排序，(f, x, y)
    // 相当于BFS的队列 但每次取的都是当前代价最小的格子
    priority_queue<tuple<int,int,int>, vector<tuple<int,int,int>>, greater<tuple<int,int,int>>> pq;
    /// @attention greater<tuple<int,int,int>> 先比较第一个元素，再比较第二个元素，再比较第三个元素
    // 曼哈顿距离启发函数
    auto heuristic = [](int x, int y) { return abs(x - endX) + abs(y - endY); };

    g[startX][startY] = 0;
    pq.push({heuristic(startX, startY), startX, startY});
    Pre[startX][startY] = {-2, -2};

    int endX_found = -1, endY_found = -1;

    while (!pq.empty() && !foundExit)
    {
        auto [f, x, y] = pq.top(); pq.pop();

        // 跳过过时条目（已找到更优路径到这个格子）
        if (f != g[x][y] + heuristic(x, y)) continue;

        for (int i = 0; i < 4; i++)
        {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= rows || ny < 0 || ny >= cols) continue;

            if (maze[nx][ny] == END)
            {
                foundExit = true;
                Pre[nx][ny] = {x, y};
                endX_found = nx; endY_found = ny;
                break;
            }
            if (maze[nx][ny] != PASSABLE) continue;

            int newG = g[x][y] + 1;
            if (newG >= g[nx][ny]) continue; // 不是更优路径

            g[nx][ny] = newG;
            Pre[nx][ny] = {x, y};
            pq.push({newG + heuristic(nx, ny), nx, ny});

            maze[nx][ny] = VISITED;
            totalVisited++;

            if (printEvery > 0 && totalVisited % printEvery == 0)
            {
                Maze::Print(maze);
                kout << "Visited:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
                Sleep(printSleep);
            }
        }
    }

    if (foundExit)
    {
        int x = endX_found, y = endY_found;
        while (Pre[x][y] != make_pair(-2, -2))
        {
            pathlen++;
            maze[x][y] = PATH;
            auto [px, py] = Pre[x][y];
            x = px; y = py;
        }
    }
}

int main()
{
    kson file = ReadKsonFile("config/algorithm/cfg.kson");
    KBegin(file["Algorithm"]["Searching"]["AStar"]);
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    printSleep = GLOBAL["MazePrintSleep"].Int();
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
    rows = (int)maze.size();
    cols = (int)maze[0].size();

    totalCells = 0;
    for (const auto& row : maze)
        for (auto cell : row)
            if (cell == PASSABLE)
                totalCells++;

    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
        {
            if (maze[r][c] == START) { startX = r; startY = c; }
            if (maze[r][c] == END)   { endX = r; endY = c; }
        }

    kout << "Solving maze (" << rows << "x" << cols << ")..." << endl;
    kout << "Start: (" << startX << "," << startY << ")" << endl;
    kout << "End:   (" << endX << "," << endY << ")" << endl;

    CheckConsoleFit(rows, cols, true);

    kout << "Print every N steps (0=no print): ";
    kin >> printEvery;
    ClearScreen();
    /// @brief 开始寻路
    AddTimer("AStar", TimeUnit::us);
    AStar();
    PauseTimer("AStar");
    /// @brief 打印结果
    ClearScreen();
    Maze::Print(maze);
    kout << endl;
    kout << "{bold}Search complete!{/}" << endl;
    kout << "Visited cells:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
    kout << "Shortest path len:{lightgray}" << pathlen << "{/}\n";
    PrintTimer("AStar");
    if (foundExit)
        kout << "{green}Exit found!{/}" << endl;
    else
        kout << "{red}No path to exit!{/}" << endl;
    KEnd();
}