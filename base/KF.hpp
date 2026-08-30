#pragma once
#include "KFCommon.hpp"
#include "KLOGGER.hpp"
#include "KMATH.hpp"
#include "KSON.hpp"
#include "KFIO.hpp"
#include "KCLI.hpp"
#include "KTIMER.hpp"
#include "KUTIL.hpp"
namespace KSON = KF::KSON;
namespace KLOG = KF::KLOGGER;
namespace KFIO = KF::KFIO;
namespace KUTIL = KF::KUTIL;
namespace KCLI = KF::KCLI;
namespace KTIMER = KF::KTIMER;
namespace KMATH = KF::KMATH;
constexpr size_t DEFAULT_RESIZE_STR_LEN = 64; // 默认KSON中字符串的分配长度 (超过这个长度会再次扩容)
using namespace KF::KLOGGER;
