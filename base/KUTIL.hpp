#pragma once
#include "KFCommon.hpp"
namespace KF
{
    /// @brief 实用库
    namespace KUTIL
    {
        sdlimb RandInt(sdlimb min, sdlimb max); ///< 生成 [min, max] 范围内的随机整数
        sdlimb Pow10(sdlimb n); // 10的n次方
        std::string MaxLenStr3(std::string a, std::string b, std::string c);
        std::string MaxLenStr4(std::string a, std::string b, std::string c, std::string d);
    }
}
