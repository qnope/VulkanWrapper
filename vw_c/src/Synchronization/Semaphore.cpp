#include "Synchronization/Semaphore.h"

#include <VulkanWrapper/Synchronization/Semaphore.h>

vw::Semaphore *vw_create_semaphore(const vw::Device *device) {
    return new vw::Semaphore{vw::SemaphoreBuilder(*device).build()};
}

void vw_destroy_semaphore(vw::Semaphore *semaphore) { delete semaphore; }
