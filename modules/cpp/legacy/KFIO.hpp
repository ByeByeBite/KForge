#pragma once
#include "KFCommon.hpp"
#include "KSON.hpp"
namespace KF
{
    namespace KFIO
    {
        std::string ReadFileRaw(std::string_view filepath);// 读取文件(粗文本 没有任何处理)

        /// @brief 从 KSON 文件读取迷宫
        /// @param filepath  KSON 文件路径
        /// @param maze_key  迷宫键名（对应 maze.kson 中的 "small" 等）
        /// @return 二维 MazeCell 网格
        std::vector<std::vector<MazeCell>> ReadMaze(
            std::string_view filepath,
            std::string_view maze_key = "small"
        );
    }
}
