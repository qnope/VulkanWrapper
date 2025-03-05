use crate::sys::bindings::{
    self, vw_Device, vw_DeviceFinder, vw_device_graphics_queue, vw_device_present_queue,
    VwDeviceCreateArguments, VwQueueFlagBits,
};
use crate::vulkan::instance::Instance;
use crate::vulkan::surface::Surface;

use super::present_queue::PresentQueue;
use super::queue::Queue;

pub struct DeviceFinder<'a> {
    _instance: &'a Instance,
    ptr: *mut vw_DeviceFinder,
    queue: VwQueueFlagBits,
    surface: Option<&'a Surface<'a>>,
    with_synchronization_2: bool,
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
                queue: VwQueueFlagBits(0),
                surface: None,
                with_synchronization_2: false,
            }
        }
    }

    pub fn with_queue(mut self, queues: bindings::VwQueueFlagBits) -> DeviceFinder<'a> {
        self.queue |= queues;
        self
    }

    pub fn with_presentation(mut self, surface: &'a Surface<'a>) -> DeviceFinder<'a> {
        self.surface = Some(surface);
        self
    }

    pub fn with_synchronization_2(mut self) -> DeviceFinder<'a> {
        self.with_synchronization_2 = true;
        return self;
    }

    pub fn build(self) -> Device<'a> {
        let ptr_surface = match self.surface {
            Some(surface) => surface.as_ptr(),
            None => 0 as *const bindings::vw_Surface,
        };
        let arguments = VwDeviceCreateArguments {
            finder: self.ptr,
            queue_flags: self.queue,
            surface_to_present: ptr_surface,
            with_synchronization_2: self.with_synchronization_2,
        };
        unsafe {
            let ptr = bindings::vw_create_device(&arguments);
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
