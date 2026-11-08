#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
namespace fw::gw::skills::effects {
// A cursor may advance one past the last uint32 instance after allocation.
// Zero additions require no instance, even when that cursor is exhausted.
inline bool effect_instance_capacity(std::uint64_t next,std::size_t count)noexcept{
    if(!count)return true;
    constexpr auto last=std::uint64_t(std::numeric_limits<std::uint32_t>::max());
    return next&&next<=last&&count<=last-next+1;
}
}
