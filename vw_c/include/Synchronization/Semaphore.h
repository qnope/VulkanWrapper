#pragma once

namespace vw {
class Semaphore;
class Device;
} // namespace vw

extern "C" {
vw::Semaphore *vw_create_semaphore(const vw::Device *device);
void vw_destroy_semaphore(vw::Semaphore *semaphore);
}
