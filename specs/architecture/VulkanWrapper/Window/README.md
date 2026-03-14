# Window Module

`vw` namespace · `Window/` directory · Tier 1

SDL3 window management and Vulkan surface creation.

## Key Types

- **SDL_Initializer** — RAII SDL initialization (`shared_ptr` for lifecycle management)
- **Window / WindowBuilder** — Window creation with builder pattern

```cpp
auto init = std::make_shared<vw::SDL_Initializer>();
auto window = vw::WindowBuilder(init)
    .with_title("App")
    .sized(Width(1024), Height(768))
    .build();
```

## Vulkan Integration

- `window.get_required_instance_extensions()` — extensions needed for surface creation
- `window.create_surface(instance)` → `Surface`
- `window.create_swapchain(device, surface)` → `Swapchain`

## Event Loop

- `window.update()` — poll events
- `window.is_close_requested()` — exit condition
- `window.is_resized()` / `window.width()` / `window.height()` — resize detection
