---
sidebar_position: 3
title: "Swapchain & Window"
---

# Swapchain & Window

The window and swapchain system handles the bridge between your rendered images and the user's screen. `Window` manages an SDL3 window with Vulkan support, `Surface` represents the drawable area within that window, and `Swapchain` manages a set of presentable images that cycle between the GPU and the display.

## Design Rationale

Presenting rendered images to the screen in Vulkan requires coordinating three separate concerns: the windowing system (platform-specific), a Vulkan surface (the bridge between window and GPU), and a swapchain (a queue of images the GPU renders into and the display reads from).

VulkanWrapper wraps SDL3 for cross-platform windowing, hides the surface creation details, and provides `Swapchain` as an RAII object that owns presentation images and their views. The `WindowBuilder` follows the same builder pattern as the rest of the library. Swapchains can be created directly from the `Window`, keeping the creation flow linear and discoverable.

Strong types `Width` and `Height` are used throughout to prevent accidental argument swaps.

## API Reference

### SDL_Initializer

```cpp
namespace vw {

class SDL_Initializer {
public:
    SDL_Initializer();
    ~SDL_Initializer();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Window/SDL_Initializer.h`

Initializes the SDL3 library. Must be created before any `Window` and must outlive all windows. It is non-copyable and non-movable. Typically created once as a `shared_ptr` at application startup:

```cpp
auto sdl_init = std::make_shared<vw::SDL_Initializer>();
```

### WindowBuilder / Window

```cpp
namespace vw {

class WindowBuilder {
public:
    WindowBuilder(std::shared_ptr<const SDL_Initializer> initializer);

    WindowBuilder &with_title(std::string_view name);
    WindowBuilder &sized(Width width, Height height);

    Window build();
};

class Window {
public:
    void update() noexcept;

    [[nodiscard]] bool is_close_requested() const noexcept;
    [[nodiscard]] bool is_resized() const noexcept;
    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;

    [[nodiscard]] static std::span<const char *const>
    get_required_instance_extensions() noexcept;

    [[nodiscard]] Surface create_surface(const Instance &instance) const;

    [[nodiscard]] Swapchain
    create_swapchain(std::shared_ptr<const Device> device,
                     vk::SurfaceKHR surface) const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Window/Window.h`

#### WindowBuilder Methods

##### `WindowBuilder(std::shared_ptr<const SDL_Initializer> initializer)`

Constructs a builder. The `SDL_Initializer` must have been created first and is kept alive via `shared_ptr`.

##### `with_title(std::string_view name)`

Sets the window title shown in the title bar. Default: `"3D Renderer"`.

##### `sized(Width width, Height height)`

Sets the window dimensions in pixels. `Width` and `Height` are strong types wrapping unsigned integers:

```cpp
builder.sized(vw::Width{1280}, vw::Height{720});
```

##### `build()`

Creates and returns the `Window`.

#### Window Methods

##### `update()`

Polls SDL events. Call this once per frame at the start of your render loop. This updates the `is_close_requested()` and `is_resized()` flags.

##### `is_close_requested()`

Returns `true` if the user has clicked the close button or pressed the platform's close shortcut. Use this to control your main loop:

```cpp
while (!window.is_close_requested()) {
    window.update();
    // render frame...
}
```

##### `is_resized()`

Returns `true` if the window was resized since the last `update()` call. When this is `true`, you must recreate the swapchain to match the new window dimensions.

##### `width()` / `height()`

Return the current window dimensions as `Width` and `Height` strong types.

##### `get_required_instance_extensions()` (static)

Returns a span of Vulkan instance extension names required for window system integration (e.g., `VK_KHR_surface` and the platform-specific surface extension). Pass these to `InstanceBuilder::addExtensions()` **before** creating the instance:

```cpp
auto extensions = vw::Window::get_required_instance_extensions();
builder.addExtensions(extensions);
```

##### `create_surface(const Instance &instance)`

Creates a Vulkan `Surface` for this window. The surface represents the drawable area where rendered images will be presented. Takes the `Instance` by const reference (not shared_ptr).

```cpp
auto surface = window.create_surface(*instance);
```

##### `create_swapchain(std::shared_ptr<const Device> device, vk::SurfaceKHR surface)`

Creates a `Swapchain` for this window using the given device and surface. The swapchain dimensions automatically match the current window size. The surface handle is obtained via `surface.handle()`.

```cpp
auto swapchain = window.create_swapchain(device, surface.handle());
```

### Surface

```cpp
namespace vw {

class Surface : public ObjectWithUniqueHandle<vk::UniqueSurfaceKHR> {
public:
    Surface(vk::UniqueSurfaceKHR surface) noexcept;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Surface.h`

A thin RAII wrapper around `vk::SurfaceKHR`. Created by `Window::create_surface()`. Access the underlying handle with `surface.handle()`. The surface is automatically destroyed when the `Surface` object goes out of scope.

### Swapchain

```cpp
namespace vw {

class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
public:
    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;
    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] std::span<const std::shared_ptr<const Image>>
    images() const noexcept;

    [[nodiscard]] std::span<const std::shared_ptr<const ImageView>>
    image_views() const noexcept;

    [[nodiscard]] int number_images() const noexcept;

    [[nodiscard]] uint64_t acquire_next_image(const Semaphore &semaphore) const;

    void present(uint32_t index, const Semaphore &waitSemaphore) const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Swapchain.h`

#### Properties

##### `width()` / `height()`

Return the swapchain's extent as `Width` and `Height` strong types.

##### `format()`

Returns the swapchain's pixel format. The format is chosen automatically based on the surface's capabilities (typically `vk::Format::eB8G8R8A8Srgb` or `vk::Format::eB8G8R8A8Unorm`).

##### `images()` / `image_views()`

Return the swapchain's backing images and their pre-built image views as spans of `shared_ptr`. The number of images depends on the present mode and surface capabilities (typically 2 or 3 for double/triple buffering). These `Image` and `ImageView` objects are created and owned by the `Swapchain`.

```cpp
auto views = swapchain.image_views();
// Use views[imageIndex] as a render target
```

##### `number_images()`

Returns the count of images in the swapchain.

#### Presentation Cycle

##### `acquire_next_image(const Semaphore &semaphore)`

Acquires the next available image from the swapchain for rendering. The provided `semaphore` is signaled when the image is ready to be rendered into. Returns the image index (use this to select the correct image view).

##### `present(uint32_t index, const Semaphore &waitSemaphore)`

Presents the rendered image at the given index to the display. Waits on `waitSemaphore` before presenting -- this semaphore should be signaled by your rendering commands when they finish writing to the image.

### SwapchainBuilder

```cpp
namespace vw {

class SwapchainBuilder {
public:
    SwapchainBuilder(std::shared_ptr<const Device> device,
                     vk::SurfaceKHR surface, Width width,
                     Height height) noexcept;

    SwapchainBuilder &with_old_swapchain(vk::SwapchainKHR old);

    Swapchain build();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Swapchain.h`

A lower-level builder for creating swapchains directly. `Window::create_swapchain()` uses this internally. The `with_old_swapchain()` method lets you pass the handle of a previous swapchain to recycle its resources during a resize.

The default present mode is `vk::PresentModeKHR::eMailbox` (low-latency triple buffering). If mailbox is not supported by the surface, it falls back to FIFO (V-sync).

### PresentQueue

```cpp
namespace vw {

class PresentQueue {
public:
    void present(const Swapchain &swapchain, uint32_t imageIndex,
                 const Semaphore &waitSemaphore) const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/PresentQueue.h`

The dedicated queue for presentation. Obtained via `device->presentQueue()`. This is an alternative to calling `swapchain.present()` directly -- both approaches work.

## Present Modes

| Mode | Behavior | V-Sync | Latency |
|------|----------|--------|---------|
| `eImmediate` | No waiting, may tear | No | Lowest |
| `eFifo` | Wait for V-blank (always supported) | Yes | Higher |
| `eFifoRelaxed` | V-sync with late frame tolerance | Partial | Medium |
| `eMailbox` | Triple buffer, no tearing (default) | Yes | Low |

The `SwapchainBuilder` defaults to `eMailbox`, which provides a good balance of low latency and no tearing. If `eMailbox` is not available, `eFifo` is always supported as a fallback.

## Usage Patterns

### Basic Window Setup

```cpp
// 1. Initialize SDL
auto sdl_init = std::make_shared<vw::SDL_Initializer>();

// 2. Create window
auto window = vw::WindowBuilder(sdl_init)
    .with_title("My Application")
    .sized(vw::Width{1280}, vw::Height{720})
    .build();

// 3. Get required extensions for the instance
auto extensions = vw::Window::get_required_instance_extensions();

// 4. Create instance with window extensions
auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .addPortability()
    .addExtensions(extensions)
    .build();

// 5. Create surface
auto surface = window.create_surface(*instance);

// 6. Create device with presentation support
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_presentation(surface.handle())
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// 7. Create swapchain
auto swapchain = window.create_swapchain(device, surface.handle());
```

### Render Loop with Presentation

```cpp
auto imageAvailable = vw::SemaphoreBuilder(device).build();
auto renderFinished = vw::SemaphoreBuilder(device).build();

while (!window.is_close_requested()) {
    window.update();

    // Acquire next swapchain image
    auto imageIndex = swapchain.acquire_next_image(imageAvailable);

    // Record rendering commands into cmd...
    // (render to swapchain.image_views()[imageIndex])

    // Submit rendering
    auto &queue = device->graphicsQueue();
    queue.enqueue_command_buffer(cmd);
    auto fence = queue.submit(
        {vk::PipelineStageFlagBits::eColorAttachmentOutput},
        {imageAvailable.handle()},
        {renderFinished.handle()});

    // Present the rendered image
    swapchain.present(imageIndex, renderFinished);

    fence.wait();
}

device->wait_idle();
```

### Handling Window Resize

When the window is resized, the swapchain must be recreated to match the new dimensions:

```cpp
while (!window.is_close_requested()) {
    window.update();

    if (window.is_resized()) {
        device->wait_idle();
        swapchain = window.create_swapchain(device, surface.handle());
        // Recreate any framebuffer-sized resources (depth buffer, etc.)
        continue;
    }

    // Normal rendering...
}
```

## Common Pitfalls

### Not calling `window.update()` each frame

**Symptom:** The window becomes unresponsive, the OS reports it as "Not Responding," the close button does not work.

**Cause:** `update()` pumps the SDL event queue. Without it, the OS thinks your application is hung because it never processes window events.

**Fix:** Call `window.update()` at the start of every frame iteration.

### Forgetting to recreate the swapchain on resize

**Symptom:** Validation errors about swapchain extent mismatch, stretched/corrupted rendering, or a crash.

**Cause:** After a resize, the old swapchain's dimensions no longer match the window. Vulkan requires the swapchain extent to match the surface's current extent.

**Fix:** Check `window.is_resized()` each frame and recreate the swapchain.

### Not waiting for the device to idle before recreating the swapchain

**Symptom:** Validation errors about destroying resources in use, or a crash.

**Cause:** The GPU may still be rendering into the old swapchain images when you attempt to destroy them by creating a new swapchain.

**Fix:** Call `device->wait_idle()` before recreating:

```cpp
if (window.is_resized()) {
    device->wait_idle();
    swapchain = window.create_swapchain(device, surface.handle());
}
```

### Not adding window extensions to the instance

**Symptom:** Surface creation fails, or the device finder cannot find a GPU with presentation support.

**Cause:** The Vulkan surface extension must be enabled at instance creation time, before any surface can be created.

**Fix:** Always pass window extensions to the instance builder before calling `build()`:

```cpp
auto extensions = vw::Window::get_required_instance_extensions();
builder.addExtensions(extensions);
```

### Using the wrong semaphore for presentation

**Symptom:** Tearing, visual artifacts, or validation errors about semaphore misuse.

**Cause:** Passing the wrong semaphore (e.g., the acquire semaphore instead of the render-finished semaphore) to `present()`.

**Fix:** Make sure the `waitSemaphore` passed to `present()` is the one that your rendering submit *signals*, not the one the acquire *signals*:

```cpp
auto imageIndex = swapchain.acquire_next_image(imageAvailable);
// imageAvailable is signaled by acquire -- wait on it during submit
auto fence = queue.submit(
    {vk::PipelineStageFlagBits::eColorAttachmentOutput},
    {imageAvailable.handle()},
    {renderFinished.handle()});
// renderFinished is signaled by submit -- wait on it during present
swapchain.present(imageIndex, renderFinished);
```

## Integration with Other Modules

| Dependency | Role | See |
|-----------|------|-----|
| `Instance` | Required for surface creation | [Instance](instance.md) |
| `Device` | Required for swapchain creation, provides present queue | [Device & GPU Selection](device.md) |
| `Semaphore` | Synchronizes acquire and present with rendering | Synchronization module |
| `Image` / `ImageView` | Swapchain images used as render targets | [Images](../image/images.md), [Image Views](../image/image-views.md) |
