from Window import SDL_Initializer
from Window import Window
import bindings_vw_py as test

initializer = SDL_Initializer.SDL_Initializer()

window = Window.WindowBuilder(initializer)\
    .sized(800, 600)\
    .with_title("From Python")\
    .build()

while not window.is_close_requested():
    window.update()

# extensions = test.CStringArray(test.vw_get_required_extensions_from_window(window))
# instanceCreate = test.VwInstanceCreateArguments()
# instanceCreate.debug_mode = True
# instanceCreate.extensions = test.VwStringArray(extensions)
# instanceCreate.extensions_count = extensions.array.size()

# test.vw_destroy_window(window)
# test.vw_destroy_instance(instance)