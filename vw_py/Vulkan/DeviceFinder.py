import bindings_vw_py as bindings
from Vulkan import Device

class DeviceFinder:
    def __init__(self, instance, device_finder):
        self.instance = instance
        self.device_finder = device_finder
        self.surface = None
        self.queue_flags = 0

    def with_queue(self, queues):
        self.queue_flags |= queues
        return self
    
    def with_presentation(self, surface):
        self.surface = surface
        return self
    
    def build(self):
        arguments = bindings.VwDeviceCreateArguments()
        arguments.finder = self.device_finder
        arguments.queue_flags = self.queue_flags
        arguments.surface_to_present = self.surface.surface
        device = bindings.vw_create_device(arguments)
        return Device.Device(self.instance, self.surface, device)
    
    def __del__(self):
        print("Remove Device Finder")
        bindings.vw_destroy_device_finder(self.device_finder)


