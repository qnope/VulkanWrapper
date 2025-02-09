#pragma once

namespace vw {
class Fence;
class Device;
} // namespace vw

extern "C" {
vw::Fence *vw_create_fence(const vw::Device *device);
void vw_wait_fence(const vw::Fence *fence);
void vw_reset_fence(const vw::Fence *fence);
void vw_destroy_fence(vw::Fence *fence);
}
