#pragma once
#include <cmath>
#include <cstdint>

namespace capture {
// Map observation does not depend on the deliberately omitted CameraModule.
inline bool valid_local_observation(bool map_observing, std::uint32_t controlled_id,
                                    std::uint16_t model, float x, float y, float facing) {
    return !map_observing && controlled_id != 0 && model == 0x3000
        && std::isfinite(x) && std::isfinite(y) && std::isfinite(facing);
}
}
