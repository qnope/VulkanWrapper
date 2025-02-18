import bindings_vw_py as bindings

class SDL_Initializer:
    initializer = bindings.vw_create_sdl_initializer()

    def __del__(self):
        print("Remove Sdl initializer")
        bindings.vw_destroy_sdl_initializer(self.initializer)