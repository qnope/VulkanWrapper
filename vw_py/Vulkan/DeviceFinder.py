import bindings_vw_py as bindings
from Vulkan import Device
from weakref import finalize

class DeviceFinder:
    def __init__(self, instance, device_finder):
        self.instance = instance
        self.device_finder = device_finder
        self.surface = None
        self.queue_flags = 0
        self.synchronization_2 = False
        finalize(self, bindings.vw_destroy_device_finder, device_finder)

    def with_queue(self, queues):
        self.queue_flags |= queues
        return self
    
    def with_presentation(self, surface):
        self.surface = surface
        return self
    
    def with_synchronization_2(self):
        self.synchronization_2 = True
        return self
    
    def build(self):
        arguments = bindings.VwDeviceCreateArguments()
        arguments.finder = self.device_finder
        arguments.queue_flags = self.queue_flags
        arguments.surface_to_present = self.surface.surface
        arguments.with_synchronization_2 = self.synchronization_2
        device = bindings.vw_create_device(arguments)
        return Device.Device(self.instance, self.surface, device)
