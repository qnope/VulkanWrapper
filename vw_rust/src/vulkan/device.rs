use crate::sys::bindings::{
    self, vw_Device, vw_DeviceFinder, vw_device_graphics_queue, vw_device_present_queue,
};
use crate::vulkan::instance::Instance;
use crate::vulkan::surface::Surface;

use super::present_queue::PresentQueue;
use super::queue::Queue;

pub struct DeviceFinder<'a> {
    _instance: &'a Instance,
    ptr: *mut vw_DeviceFinder,
    queue: bindings::VkQueueFlagBits,
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
                queue: bindings::VkQueueFlagBits(0),
                surface: None,
            }
        }
    }

    pub fn with_queue(mut self, queues: bindings::VkQueueFlagBits) -> DeviceFinder<'a> {
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
            let ptr = bindings::vw_create_device(self.ptr, self.queue, ptr_surface);
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

    pub fn graphics_queue(&'a self) -> Queue<'a> {
        Queue::new(self, unsafe { vw_device_graphics_queue(self.ptr) })
    }

    pub fn present_queue(&'a self) -> PresentQueue<'a> {
        PresentQueue::new(self, unsafe { vw_device_present_queue(self.ptr) })
    }

    pub fn wait_idle(&self) {
        unsafe {
            bindings::vw_device_wait_idle(self.ptr);
        }
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
