#include "utils/JsonUtils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <iostream>
#include <map>

// JsonUtils 的头文件中所有方法都是 static inline 定义的,
// 这个 .cpp 文件提供一些额外的全局辅助函数。
// 注意: 头文件 (yachiyo::utils::JsonUtils) 是 header-only，
// 所有 static 方法已在头文件中实现。

namespace yachiyo {
namespace utils {

// ==================== 额外静态工具函数 (不在头文件中) ====================
// 以下函数是补充性的，主要供内部使用

} // namespace utils
} // namespace yachiyo