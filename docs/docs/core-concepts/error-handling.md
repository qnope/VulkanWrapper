---
sidebar_position: 5
---

# Error Handling

VulkanWrapper uses exceptions for error reporting. Every exception captures the source location where the error originated, making it straightforward to trace failures back to the exact line of code.

## Exception Hierarchy

All exceptions inherit from `vw::Exception`:

```
vw::Exception                          base -- message + source_location
├── VulkanException                    Vulkan API errors (carries vk::Result)
├── VMAException                       VMA allocation errors
├── SDLException                       SDL errors (auto-captures SDL_GetError())
├── FileException                      File I/O errors
├── AssimpException                    Model loading errors
├── ShaderCompilationException         GLSL compilation errors (shader name, stage, log)
├── ValidationLayerException           Vulkan validation layer errors
├── SwapchainException                 Swapchain-specific errors (recoverable)
└── LogicException                     Programming errors
    ├── out_of_range()                 Index out of bounds
    ├── invalid_state()                Invalid object state
    └── null_pointer()                 Unexpected null pointer
```

## The Base Exception

Every VulkanWrapper exception inherits from `vw::Exception`, which extends `std::exception` with automatic source location capture:

```cpp
class Exception : public std::exception {
public:
    Exception(std::string message,
              std::source_location location = std::source_location::current());

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const std::source_location& location() const noexcept;
};
```

The `location` parameter has a default value, so it is captured automatically at the throw site -- you never need to pass it manually. This means every exception knows its file, line, column, and function name.

## Helper Functions

VulkanWrapper provides three helper functions that check results and throw the appropriate exception on failure.

### check_vk

Checks a Vulkan result and throws `VulkanException` if it indicates an error:

```cpp
auto result = device.createBuffer(&createInfo, nullptr, &buffer);
check_vk(result, "create buffer");
```

The second argument is a context string that appears in the exception message, helping you identify which Vulkan call failed.

For `vk::ResultValue<T>` returns, `check_vk` unwraps the value on success:

```cpp
auto fence = check_vk(
    device.createFenceUnique(createInfo),
    "create fence"
);
// fence is now a vk::UniqueFence, not a ResultValue
```

### check_sdl

Checks SDL operations. It has two overloads -- one for booleans and one for pointers:

```cpp
// Boolean overload
bool success = SDL_Init(SDL_INIT_VIDEO);
check_sdl(success, "initialize SDL");

// Pointer overload -- throws if nullptr
SDL_Window* window = SDL_CreateWindow("Title", w, h, flags);
check_sdl(window, "create window");
```

`SDLException` automatically calls `SDL_GetError()` to capture SDL's error message.

### check_vma

Checks VMA allocation results:

```cpp
VkResult result = vmaCreateBuffer(
    allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr
);
check_vma(result, "allocate buffer");
```

## Exception Types in Detail

### VulkanException

Thrown when a Vulkan API call returns an error result. Carries the `vk::Result` value:

```cpp
try {
    // ... Vulkan operations ...
} catch (const VulkanException& e) {
    std::cerr << "Vulkan error: " << e.what() << '\n';
    std::cerr << "Result code: " << vk::to_string(e.result()) << '\n';
    std::cerr << "Location: " << e.location().file_name()
              << ":" << e.location().line() << '\n';
}
```

### SDLException

Thrown when an SDL operation fails. The constructor automatically calls `SDL_GetError()` and includes the SDL error string in the message:

```cpp
try {
    check_sdl(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");
} catch (const SDLException& e) {
    // e.what() includes both "initialize SDL" and SDL_GetError() output
    std::cerr << e.what() << '\n';
}
```

### VMAException

Thrown when a VMA allocation or operation fails. Similar to `VulkanException` but specific to VMA.

### FileException

Thrown for file I/O errors (e.g., shader file not found, texture file unreadable).

### AssimpException

Thrown when Assimp fails to load or parse a 3D model file.

### ShaderCompilationException

Thrown when a GLSL shader fails to compile. Carries detailed diagnostic information:

```cpp
try {
    auto shader = ShaderCompiler().compile_file_to_module(device, "broken.frag");
} catch (const ShaderCompilationException& e) {
    std::cerr << "Shader: " << e.shader_name() << '\n';
    std::cerr << "Stage: " << e.stage() << '\n';
    std::cerr << "Errors:\n" << e.compilation_log() << '\n';
}
```

- `shader_name()` -- the file path or name of the shader that failed.
- `stage()` -- the shader stage (vertex, fragment, etc.).
- `compilation_log()` -- the full compiler error output with line numbers.

### ValidationLayerException

Thrown when a Vulkan validation layer reports an error. This typically happens during development when validation layers are enabled via `InstanceBuilder().setDebug()`.

### SwapchainException

A special recoverable exception for swapchain-related errors. Unlike most exceptions, these often indicate conditions you should handle gracefully rather than abort:

```cpp
try {
    // Present or acquire next image...
} catch (const SwapchainException& e) {
    if (e.is_out_of_date()) {
        // Window was resized -- recreate the swapchain
        recreate_swapchain();
    } else if (e.is_suboptimal()) {
        // Swapchain still works but could be better
        // You can choose to recreate or continue
    }

    // General check: does this need swapchain recreation?
    if (e.needs_recreation()) {
        recreate_swapchain();
    }
}
```

Methods:
- `needs_recreation()` -- returns `true` if the swapchain must be recreated to continue.
- `is_out_of_date()` -- the swapchain is no longer compatible with the surface (e.g., after a resize).
- `is_suboptimal()` -- the swapchain still works but no longer matches the surface properties optimally.

### LogicException

Represents programming errors -- situations that should never happen in correct code. Created via static factory methods:

```cpp
// Index out of bounds
throw LogicException::out_of_range("buffer index exceeds size");

// Object in an invalid state
throw LogicException::invalid_state("pipeline not yet built");

// Unexpected null pointer
throw LogicException::null_pointer("device was null");
```

These are distinct from Vulkan errors because they indicate bugs in your code rather than runtime failures.

## Error Handling Patterns

### Let RAII Handle Cleanup

Because VulkanWrapper uses RAII, you do not need `try/catch` blocks just for cleanup. Resources are freed automatically when exceptions propagate:

```cpp
void create_resources(Allocator& allocator) {
    auto buffer1 = create_buffer<float, true, UniformBufferUsage>(allocator, 100);
    auto buffer2 = create_buffer<float, true, UniformBufferUsage>(allocator, 200);
    // If buffer2 throws, buffer1 is automatically freed -- no catch needed
}
```

### Catch Specific Exceptions

When you can recover from a specific error, catch that exception type:

```cpp
try {
    auto device = instance->findGpu()
        .with_queue(vk::QueueFlagBits::eGraphics)
        .with_ray_tracing()
        .build();
} catch (const VulkanException& e) {
    if (e.result() == vk::Result::eErrorExtensionNotPresent) {
        std::cerr << "Ray tracing not supported, using fallback\n";
        // Build device without ray tracing
    } else {
        throw;  // Re-throw errors you cannot handle
    }
}
```

### Use Source Location for Debugging

Every exception carries a `std::source_location`. Use it in error reports:

```cpp
catch (const vw::Exception& e) {
    std::cerr << e.what() << '\n';
    std::cerr << "  in " << e.location().function_name() << '\n';
    std::cerr << "  at " << e.location().file_name()
              << ":" << e.location().line() << '\n';
}
```

### Enable Validation Layers During Development

Validation layers catch Vulkan misuse (wrong image layouts, missing barriers, invalid parameters) and report it as `ValidationLayerException` or debug output. Always enable them during development:

```cpp
auto instance = InstanceBuilder()
    .setDebug()                          // enable validation layers
    .setApiVersion(ApiVersion::e13)
    .build();
```

Disable them in release builds for better performance.

## Summary

| Exception | Source | Recoverable? | Key Data |
|-----------|--------|-------------|----------|
| `VulkanException` | `check_vk` | Rarely | `result()` |
| `SDLException` | `check_sdl` | Rarely | SDL error string |
| `VMAException` | `check_vma` | Rarely | VMA error code |
| `FileException` | File I/O | Sometimes | File path |
| `AssimpException` | Model loading | Sometimes | Model path |
| `ShaderCompilationException` | Shader compile | Fix and retry | `shader_name()`, `stage()`, `compilation_log()` |
| `ValidationLayerException` | Validation layers | Fix the bug | Validation message |
| `SwapchainException` | Present/acquire | Yes | `needs_recreation()`, `is_out_of_date()`, `is_suboptimal()` |
| `LogicException` | Programming errors | Fix the bug | `out_of_range()`, `invalid_state()`, `null_pointer()` |
