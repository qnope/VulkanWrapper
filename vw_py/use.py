import bindings_vw_py as test
import ctypes

sdl_initializer = test.vw_create_sdl_initializer()

windowCreate = test.VwWindowCreateArguments()
windowCreate.width = 800
windowCreate.height = 600
windowCreate.title = "Test from Python"
windowCreate.initializer = sdl_initializer

window = test.vw_create_window(windowCreate)

extensions, number = test.vw_get_required_extensions_from_window(window)
instanceCreate = test.VwInstanceCreateArguments()
instanceCreate.debug_mode = True
instanceCreate.extensions = extensions
instanceCreate.extensions_count = number

instance = test.vw_create_instance(instanceCreate)


while not test.vw_is_close_window_requested(window):
    test.vw_update_window(window)

test.vw_destroy_window(window)
test.vw_destroy_sdl_initializer(sdl_initializer)
test.vw_destroy_instance(instance)