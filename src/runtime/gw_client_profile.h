#pragma once
#include <cstdint>
#include <optional>

namespace fw::gw {
// Descriptor matches in the supplied executables; not a claim that every
// packet or portal ABI is interchangeable between these builds.
struct ClientProfile {
    std::uint32_t build;
    std::uint16_t map_head, map_name, map_finish, map_spawn, instance_info;
};
inline constexpr ClientProfile client_2022{37130,379,380,397,404,408};
inline constexpr ClientProfile client_reforged{38888,380,381,398,405,409};
inline constexpr std::optional<ClientProfile> client_profile(std::uint32_t build) {
    if (build==client_2022.build) return client_2022;
    if (build==client_reforged.build) return client_reforged;
    return std::nullopt;
}
static_assert(client_profile(37130)->instance_info==408);
static_assert(client_profile(38888)->map_finish==398);
static_assert(!client_profile(0));
}
