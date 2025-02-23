import bindings_vw_py as bindings
from Vulkan import GraphicsQueue, PresentQueue

class Device:
    def __init__(self, instance, surface, device):
        self.instance = instance
        self.surface = surface
        self.device = device

    def graphics_queue(self):
        return GraphicsQueue.GraphicsQueue(self.device, bindings.vw_device_graphics_queue(self.device))

    def present_queue(self):
        return PresentQueue.PresentQueue(self.device, bindings.vw_device_present_queue(self.device))
    
    def wait_idle(self):
        bindings.vw_device_wait_idle(self.device)

    def __del__(self):
        bindings.vw_destroy_device(self.device)
        print("Remove Device")