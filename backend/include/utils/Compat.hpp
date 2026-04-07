#pragma once

/**
 * @file Compat.hpp
 * @brief 命名空间兼容性别名
 * 
 * 解决 Yachiyo::* (大写) 与 yachiyo::* (小写) 命名空间之间的引用问题。
 * 在 yachiyo::services 等命名空间中，可以通过 Utils:: 和 dto:: 访问类型。
 */

// 确保核心命名空间已定义
namespace Yachiyo { namespace Utils {} }
namespace Yachiyo { namespace DTO {} }
namespace Yachiyo { namespace Services {} }

namespace yachiyo {
    // 使得在 yachiyo:: 及其子命名空间中可以用 Utils:: 访问 Yachiyo::Utils
    namespace Utils = ::Yachiyo::Utils;
}

// 注意: yachiyo::dto 已经是一个真实命名空间 (在 CommonDTO/AuthDTO 等中定义了向后兼容 using 声明)
// 而 Yachiyo::DTO 是主命名空间 (大写)
// 从 yachiyo::services 中访问 dto::XXX 时，会通过 yachiyo::dto 中的 using 声明找到 Yachiyo::DTO 中的类型
// 但对于没有被 using 到 yachiyo::dto 中的类型，需要在 services 中直接使用 Yachiyo::DTO::XXX
