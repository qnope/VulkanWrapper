use crate::sys::bindings::{self, vw_Device, vw_DeviceFinder};
use crate::vulkan::instance::Instance;
use crate::vulkan::surface::Surface;
use bitflags::bitflags;

bitflags! {
    #[derive(Clone, Copy)]
    pub struct QueueFlags : u32 {
        const Compute = bindings::VkQueueFlagBits_VK_QUEUE_COMPUTE_BIT;
        const Transfer = bindings::VkQueueFlagBits_VK_QUEUE_TRANSFER_BIT;
        const Graphics = bindings::VkQueueFlagBits_VK_QUEUE_GRAPHICS_BIT;
}
}
pub struct DeviceFinder<'a> {
    _instance: &'a Instance,
    ptr: *mut vw_DeviceFinder,
    queue: QueueFlags,
    surface: Option<&'a Surface<'a>>,
}

pub struct Device<'a> {
    _instance: &'a Instance,
    _surface: Option<&'a Surface<'a>>,
    ptr: *mut vw_Device,
}

impl<'a> DeviceFinder<'a> {
    pub fn new(instance: &'a Instance) -> DeviceFinder<'a> {
        unsafe {
            DeviceFinder {
                _instance: &instance,
                ptr: bindings::vw_find_gpu_from_instance(instance.as_ptr()),
                queue: QueueFlags::empty(),
                surface: None,
            }
        }
    }

    pub fn with_queue(mut self, queues: QueueFlags) -> DeviceFinder<'a> {
        self.queue = self.queue | queues;
        self
    }

    pub fn with_presentation(mut self, surface: &'a Surface<'a>) -> DeviceFinder<'a> {
        self.surface = Some(surface);
        self
    }

    pub fn build(self) -> Device<'a> {
        let ptr_surface = match self.surface {
            Some(surface) => surface.as_ptr(),
            None => 0 as *const bindings::vw_Surface,
        };
        unsafe {
            let ptr = bindings::vw_create_device(self.ptr, self.queue.bits(), ptr_surface);
            Device {
                _instance: self._instance,
                _surface: self.surface,
                ptr: ptr,
            }
        }
    }
}

impl<'a> Device<'a> {
    pub fn as_ptr(&self) -> *const vw_Device {
        self.ptr
    }
}

impl<'a> Drop for DeviceFinder<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_device_finder(self.ptr);
        }
    }
}

impl<'a> Drop for Device<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_device(self.ptr);
        }
    }
}
