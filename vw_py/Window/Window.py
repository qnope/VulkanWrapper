import bindings_vw_py as bindings
from Vulkan import Surface, Swapchain

class Window:
    def __init__(self, initializer, window):
        self.initializer = initializer
        self.window = window

    def is_close_requested(self):
        return bindings.vw_is_close_window_requested(self.window)
    
    def update(self):
        bindings.vw_update_window(self.window)

    def get_required_extensions(self):
        return bindings.vw_get_required_extensions_from_window(self.window)
    
    def create_surface(self, instance):
        surface = bindings.vw_create_surface_from_window(self.window, instance.instance)
        return Surface.Surface(instance, self, surface)
    
    def create_swapchain(self, device, surface):
        swapchain = bindings.vw_create_swapchain_from_window(self.window, device.device, surface.surface)
        return Swapchain.Swapchain(device, surface, swapchain)

    def __del__(self):
        bindings.vw_destroy_window(self.window)
        print("Remove Window")

class WindowBuilder:
    width = 0
    height = 0
    title = bindings.CString("")

    def __init__(self, initializer):
        self.initializer = initializer

    def sized(self, width, height):
        self.width = width
        self.height = height
        return self
    
    def with_title(self, title):
        self.title = bindings.CString(title)
        return self
    
    def build(self):
        arguments = bindings.VwWindowCreateArguments()
        arguments.initializer = self.initializer.initializer
        arguments.height = self.height
        arguments.width = self.width
        arguments.title = bindings.VwString(self.title)
        window = bindings.vw_create_window(arguments)
        return Window(self.initializer, window)