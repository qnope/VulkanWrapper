#pragma once

#include <cstdint>

namespace vw {

enum class Slot {
    // Geometry
    Depth,

    // G-Buffer
    Albedo,
    Normal,
    Tangent,
    Bitangent,
    Position,
    DirectLight,
    IndirectRay,

    // Post-process
    AmbientOcclusion,
    Sky,
    IndirectLight,

    // Final
    ToneMapped,

    // User extension point
    UserSlot = 1024
};

} // namespace vw
