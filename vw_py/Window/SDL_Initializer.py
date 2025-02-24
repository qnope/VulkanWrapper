import bindings_vw_py as bindings
from weakref import finalize

class SDL_Initializer:
    def __init__(self):
        self.initializer = bindings.vw_create_sdl_initializer()
        finalize(self, bindings.vw_destroy_sdl_initializer, self.initializer)