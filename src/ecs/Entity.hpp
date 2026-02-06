#pragma once

#include <entt/entt.hpp>

namespace gloaming {

/// Entity handle - wrapper around EnTT entity for convenience
using Entity = entt::entity;

/// Null entity constant
constexpr Entity NullEntity = entt::null;

} // namespace gloaming
