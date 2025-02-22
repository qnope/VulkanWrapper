import bindings_vw_py as bindings

class Swapchain:
    def __init__(self, device, surface, swapchain): 
        self.device = device
        self.surface = surface
        self.swapchain = swapchain

    def __del__(self):
        print("Remove Swapchain")
        bindings.vw_destroy_swapchain(self.swapchain)