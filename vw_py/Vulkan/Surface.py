import bindings_vw_py as bindings
from weakref import finalize

class Surface:
    def __init__(self, instance, window, surface):
        self.instance = instance
        self.window = window
        self.surface = surface
        finalize(self, bindings.vw_destroy_surface, surface)