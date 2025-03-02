#pragma once

namespace vw {
class Fence;
class Device;
} // namespace vw

extern "C" {
void vw_wait_fence(const vw::Fence *fence);
void vw_reset_fence(const vw::Fence *fence);
void vw_destroy_fence(vw::Fence *fence);
}
