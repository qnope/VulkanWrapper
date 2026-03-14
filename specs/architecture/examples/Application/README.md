# Application Example

`examples/Application/` directory

Minimal Vulkan application setup showing the full initialization chain.

## App Class

The `App` class initializes in declaration order (C++ member init):

```cpp
class App {
    shared_ptr<SDL_Initializer> initializer;     // 1. SDL init
    Window window;                                // 2. Window (1024x768)
    shared_ptr<Instance> instance;                // 3. Vulkan instance (debug, API 1.3, portability)
    Surface surface;                              // 4. Window surface
    shared_ptr<Device> device;                    // 5. GPU selection (graphics+compute+transfer,
                                                  //    presentation, sync2, RT, dynamic rendering,
                                                  //    descriptor indexing)
    shared_ptr<Allocator> allocator;              // 6. VMA allocator
    Swapchain swapchain;                          // 7. Swapchain
};
```

Demonstrates the builder pattern chain:

```
InstanceBuilder → DeviceFinder → AllocatorBuilder → SwapchainBuilder
```

All fields are `public` — the App struct is a convenience container for examples, not an encapsulated API.
