import bindings_vw_py as bindings
from Vulkan import Surface, Swapchain
from weakref import finalize

class Window:
    def __init__(self, initializer, window):
        self.initializer = initializer
        self.window = window
        finalize(self, bindings.vw_destroy_window, window)

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

class WindowBuilder:
    def __init__(self, initializer):
        self.initializer = initializer
        self.width = 0
        self.height = 0
        self.title = bindings.CString("")

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