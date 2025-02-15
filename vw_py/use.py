import bindings_vw_py as test

sdl_initializer = test.vw_create_sdl_initializer()

windowCreate = test.VwWindowCreateArguments()
windowCreate.width = 800
windowCreate.height = 600
windowCreate.title = "Test from Python"
windowCreate.initializer = sdl_initializer

window = test.vw_create_window(windowCreate)

while not test.vw_is_close_window_requested(window):
    test.vw_update_window(window)

test.vw_destroy_window(window)
test.vw_destroy_sdl_initializer(sdl_initializer)