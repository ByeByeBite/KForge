#define _CRT_SECURE_NO_WARNINGS
#include "base/KF.hpp"

/**
 * @file KFIO.cpp
 * @brief KFIO 文件读取模块
 * @version 1.0.0
 * @date 2026-08-13
 * @author Git-1145
 */


namespace KF
{
    namespace KFIO
    {
        /// @brief 把相对/绝对路径解析为基于当前工作目录的绝对路径（用于失败诊断）
        static std::string ToAbsolute(std::string_view path)
        {
            char buf[MAX_PATH] = {0};
            DWORD len = GetFullPathNameA(std::string(path).c_str(), MAX_PATH, buf, nullptr);
            // 返回 0 或超出缓冲长度则回退用原始路径，保证不丢信息
            if (len == 0 || len >= MAX_PATH)
                return std::string(path);
            return std::string(buf);
        }

        /// @brief 判断路径是否为相对路径
        /// @retval true  相对路径（需要拼接项目根目录）
        /// @retval false 绝对路径（如 C:\... 或 \\...）
        static bool IsRelativePath(std::string_view path)
        {
            if (path.empty()) return true;
            // 绝对路径：X:\... 或 \...（含 \\server\share）
            if (path.size() >= 3 && path[1] == ':' && path[2] == '\\')
                return false;
            if (path[0] == '\\')
                return false;
            return true;
        }

        /// @brief 自动检测项目根目录（从 exe 路径向上查找 base\KF.hpp）
        /// @return 项目根目录的绝对路径，末尾不带反斜杠
        /// @note  仅在首次调用时扫描一次，结果缓存
        static const std::string& GetProjectRoot()
        {
            static const std::string root = []() -> std::string
            {
                char buf[MAX_PATH];
                DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
                if (len == 0 || len >= MAX_PATH)
                    return "";

                std::string dir(buf);
                size_t pos = dir.find_last_of('\\');
                if (pos == std::string::npos)
                    return "";
                dir = dir.substr(0, pos);

                // 逐层向上查找 base\KF.hpp 标记文件
                while (!dir.empty())
                {
                    std::string marker = dir + "\\base\\KF.hpp";
                    DWORD attr = GetFileAttributesA(marker.c_str());
                    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
                        return dir;

                    size_t sep = dir.find_last_of('\\');
                    if (sep == std::string::npos)
                        break;
                    dir = dir.substr(0, sep);
                }
                return "";
            }();
            return root;
        }

        /// @brief 读取文件（粗文本，不做任何处理）
        /// @param filepath 文件路径（相对路径自动拼接项目根目录）
        /// @return 文件全部内容的 std::string
        /// @note  失败时会 Fatal 并附带绝对路径与 errno，便于定位
        ///        @example std::string text = KFIO::ReadFileRaw("config/config.kson");
        std::string ReadFileRaw(std::string_view filepath)
        {
            // 相对路径 → 拼接项目根目录
            std::string fullPath;
            if (IsRelativePath(filepath))
            {
                const std::string& root = GetProjectRoot();
                if (!root.empty())
                    fullPath = root + "\\" + std::string(filepath);
                else
                    fullPath = std::string(filepath);
            }
            else
            {
                fullPath = std::string(filepath);
            }

            std::ifstream File(fullPath, std::ios::binary);
            if (!File)
            {
                // 立即取 errno，避免被后续调用覆盖
                int err = errno;
                std::string absPath = ToAbsolute(filepath);
                std::string extra = absPath + " | errno=" + std::to_string(err)
                                  + ": " + std::strerror(err);
                KLOG_FATAL(KFIO_FILE_OPEN_FAIL, extra);
            }

            // 获取文件大小，预分配内存
            File.seekg(0, std::ios::end);
            std::size_t size = static_cast<std::size_t>(File.tellg());
            File.seekg(0, std::ios::beg);

            // 一次性读取整个文件
            std::string content;
            content.resize(size);
            File.read(content.data(), static_cast<std::streamsize>(size));

            if (!File)
            {
                int err = errno;
                std::string extra = std::string(filepath) + " | errno=" + std::to_string(err)
                                  + ": " + std::strerror(err);
                KLOG_FATAL(KFIO_FILE_READ_FAIL, extra);
            }

            // 后处理：过滤制表符（\t）
            std::string filtered;
            filtered.reserve(content.size());
            for (std::size_t i = 0; i < content.size(); ++i)
            {
                if (content[i] != '\t')
                    filtered += content[i];
            }
            return filtered;
        }

        /// @brief 从 KSON 文件读取迷宫
        std::vector<std::vector<MazeCell>> ReadMaze(
            std::string_view filepath,
            std::string_view maze_key)
        {
            // 读取并解析 KSON
            std::string raw = ReadFileRaw(filepath);
            KSON::kson doc = KSON::read(KSON::Preprocess(raw));
            KSON::kson mazeNode = doc["maze"][maze_key];

            std::vector<std::vector<MazeCell>> grid;

            if (mazeNode.Resolve() && mazeNode.Resolve()->IsObject())
            {
                // 新格式：RLE 压缩单行
                int w = (int)mazeNode["w"].Int();
                std::string rle = mazeNode["data"].Str();

                // RLE 解码
                std::string decoded;
                decoded.reserve(rle.size() * 2);
                size_t i = 0;
                while (i < rle.size())
                {
                    char c = rle[i++];
                    int count = 0;
                    while (i < rle.size() && isdigit((unsigned char)rle[i]))
                    {
                        count = count * 10 + (rle[i] - '0');
                        i++;
                    }
                    decoded.append(count, c);
                }

                // 按宽度分行
                int rows = (int)decoded.size() / w;
                grid.reserve(rows);
                for (int r = 0; r < rows; r++)
                {
                    std::vector<MazeCell> row;
                    row.reserve(w);
                    for (int c = 0; c < w; c++)
                    {
                        char ch = decoded[r * w + c];
                        if      (ch == MAZE_WALL)  row.push_back(MazeCell::WALL);
                        else if (ch == MAZE_PATH)  row.push_back(MazeCell::PASSABLE);
                        else if (ch == MAZE_START) row.push_back(MazeCell::START);
                        else if (ch == MAZE_END)   row.push_back(MazeCell::END);
                        else                       row.push_back(MazeCell::PASSABLE);
                    }
                    grid.push_back(std::move(row));
                }
            }
            else
            {
                // 旧格式：数组
                size_t rows = mazeNode.Size();
                grid.reserve(rows);
                for (size_t r = 0; r < rows; r++)
                {
                    std::string rowStr = mazeNode[r][size_t(0)].Str();
                    std::vector<MazeCell> row;
                    row.reserve(rowStr.size());
                    for (char c : rowStr)
                    {
                        if      (c == MAZE_WALL)  row.push_back(MazeCell::WALL);
                        else if (c == MAZE_PATH)  row.push_back(MazeCell::PASSABLE);
                        else if (c == MAZE_START) row.push_back(MazeCell::START);
                        else if (c == MAZE_END)   row.push_back(MazeCell::END);
                        else                 row.push_back(MazeCell::PASSABLE);
                    }
                    grid.push_back(std::move(row));
                }
            }
            return grid;
        }
    }
}
