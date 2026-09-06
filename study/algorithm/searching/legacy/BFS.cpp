#include "modules/cpp/KF.hpp"
using namespace std;
using namespace KFIO;
using namespace KSON;
using namespace KCLI;
using namespace KF;
using namespace KTIMER;
size_t totalVisited = 0;      // 已探索格子数
size_t totalCells = 0;        // 迷宫 P 总数
size_t printEvery = 0;        // 每打印一次间隔的步数（用户输入），0=不打印
static long long printSleep;  // 每次打印后的停顿(ms)，来自 GLOBAL["MazePrintSleep"]
bool foundExit = false;
vector<vector<MazeCell>> maze;
size_t rows,cols;
int startX = -1, startY = -1, endX = -1, endY = -1;
size_t pathlen = 0;//最短路径长度
void BFS(size_t Row, size_t Col)//迷宫和起点终点都是全局变量
{
    /// @brief 初始化

    int dx[] = {0,0,1,-1};
    int dy[] = {1,-1,0,0};
    queue<pair<int,int>> q;
    vector<vector<pair<int,int>>> Pre(Row, vector<pair<int,int>>(Col, {-1,-1}));
    q.push({startX, startY});
    Pre[startX][startY] = {-2, -2};

    int endX_found = -1, endY_found = -1;
    while (!q.empty())
    {
        auto [x, y] = q.front(); q.pop();
        for (int i = 0; i < 4; i++)
        {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= Row || ny < 0 || ny >= Col) continue;
            if (maze[nx][ny] == END) { foundExit = true; Pre[nx][ny] = {x, y}; endX_found = nx; endY_found = ny; break; }
            if (maze[nx][ny] != PASSABLE || Pre[nx][ny] != make_pair(-1,-1)) continue;
            Pre[nx][ny] = {x, y};
            q.push({nx, ny});
            maze[nx][ny] = VISITED;
            totalVisited++;
            if (printEvery > 0 && totalVisited % printEvery == 0)
            {
                Maze::Print(maze);
                kout << "Visited:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
                Sleep(printSleep);
            }
        }
        if (foundExit) break;
    }
    if (foundExit)
    {
        int x = endX_found, y = endY_found;
        while (Pre[x][y] != make_pair(-2,-2))
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
    KBegin(file["Algorithm"]["Searching"]["BFS"]);
    printSleep = GLOBAL["MazePrintSleep"].Int();
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    /// @brief 寻找可用的迷宫
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
    /// @brief 选择迷宫
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
    rows = (int)maze.size(); //行
    cols = (int)maze[0].size(); //列

    totalCells = 0;
    for (const auto& row : maze)
        for (auto cell : row)
            if (cell == PASSABLE)
                totalCells++;

    for (size_t r = 0; r < rows; r++)
        for (size_t c = 0; c < cols; c++)
        {
            if (maze[r][c] == START) { startX = r; startY = c; }
            if (maze[r][c] == END)   { endX = r; endY = c; }
        }
    vector<vector<int>> dist(rows, vector<int>(cols));
    kout << "Solving maze (" << rows << "x" << cols << ")..." << endl;
    kout << "Start: (" << startX << "," << startY << ")" << endl;
    kout << "End:   (" << endX << "," << endY << ")" << endl;

    CheckConsoleFit((int)rows, (int)cols, true);
    /// @brief 设置打印间隔
    kout << "Print every N steps (0=no print): ";
    kin >> printEvery;
    ClearScreen();
    /// @brief 开始搜索
    AddTimer("BFS", TimeUnit::us);
    BFS(rows,cols);
    PauseTimer("BFS");
    /// @brief 打印结果
    ClearScreen();
    Maze::Print(maze);
    kout << endl;
    kout << "{bold}Search complete!{/}" << endl;
    kout << "Visited cells:{lightgray}" << totalVisited << "{/} / " << totalCells << endl;
    kout << "Shortest path len:{lightgray}" << pathlen << "{/}\n";
    PrintTimer("BFS");
    if (foundExit)
        kout << "{green}Exit found!{/}" << endl;
    else
        kout << "{red}No path to exit!{/}" << endl;
    KEnd();
}