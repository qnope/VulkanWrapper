use super::swapchain::Swapchain;
use crate::synchronization::semaphore::Semaphore;
use crate::sys::bindings::{self, vw_PresentQueue, VwPresentQueueArguments};
use crate::vulkan::device::Device;
use std::marker::PhantomData;

pub struct PresentQueue<'a> {
    ptr: *const vw_PresentQueue,
    _marker: PhantomData<&'a Device<'a>>,
}

impl<'a> PresentQueue<'a> {
    pub fn new(_device: &'a Device<'a>, ptr: *const vw_PresentQueue) -> PresentQueue<'a> {
        PresentQueue {
            ptr: ptr,
            _marker: PhantomData,
        }
    }

    pub fn present(
        &self,
        swapchain: &'a Swapchain<'a>,
        image_index: u64,
        wait_semaphore: &'a Semaphore<'a>,
    ) {
        let arguments = VwPresentQueueArguments {
            swapchain: swapchain.as_ptr(),
            image_index: image_index as i32,
            wait_semaphore: wait_semaphore.as_ptr(),
        };
        unsafe {
            bindings::vw_present_queue_present(self.ptr, &arguments);
        }
    }
}
