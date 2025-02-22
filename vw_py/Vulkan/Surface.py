import bindings_vw_py as bindings

class Surface:
    def __init__(self, instance, window, surface):
        self.instance = instance
        self.window = window
        self.surface = surface

    def __del__(self):
        bindings.vw_destroy_surface(self.surface)
        print("Remove Surface")