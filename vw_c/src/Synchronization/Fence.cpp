#include "Synchronization/Fence.h"

#include <VulkanWrapper/Synchronization/Fence.h>

void vw_wait_fence(const vw::Fence *fence) { fence->wait(); }

void vw_reset_fence(const vw::Fence *fence) { fence->reset(); }

void vw_destroy_fence(vw::Fence *fence) { delete fence; }
