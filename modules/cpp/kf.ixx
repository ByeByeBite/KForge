export module kf;

// 组合模块：kf 仅依赖这几个模块，符号已去掉命名空间、平铺到全局
// kf 不含大数模块；需要大数时单独 import kbignum
export import klogger;
export import kson;
export import kcli;
export import ktimer;
export import kutil;