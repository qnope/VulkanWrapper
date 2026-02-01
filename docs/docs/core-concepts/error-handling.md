---
sidebar_position: 5
---

# Error Handling

VulkanWrapper uses exceptions for error handling, providing detailed error information including source location.

## Exception Hierarchy

All exceptions inherit from `vw::Exception`:

```
vw::Exception                        # Base - message + source_location
├── VulkanException                  # Vulkan API errors (vk::Result)
├── SDLException                     # SDL errors (auto-captures SDL_GetError)
├── VMAException                     # VMA allocation errors
├── FileException                    # File I/O errors
├── AssimpException                  # Model loading errors
├── ShaderCompilationException       # GLSL compilation errors
└── LogicException                   # Logic/state errors
    ├── out_of_range
    ├── invalid_state
    └── null_pointer
```

## Base Exception

The base `Exception` class captures the source location automatically:

```cpp
class Exception : public std::exception {
public:
    Exception(std::string message,
              std::source_location location = std::source_location::current());

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const std::source_location& location() const noexcept;

private:
    std::string m_message;
    std::source_location m_location;
};
```

## Helper Functions

### check_vk

Check Vulkan results and throw on failure:

```cpp
// Check a vk::Result
vk::Result result = device.createBuffer(&createInfo, nullptr, &buffer);
check_vk(result, "create buffer");

// Check a std::pair<vk::Result, T>
auto [result, value] = device.createBufferUnique(createInfo);
auto buffer = check_vk(std::move(result), std::move(value), "create buffer");

// Check a vk::ResultValue<T>
auto resultValue = device.createFenceUnique(createInfo);
auto fence = check_vk(std::move(resultValue), "create fence");
```

### check_sdl

Check SDL operations:

```cpp
// Check a boolean result
bool success = SDL_Init(SDL_INIT_VIDEO);
check_sdl(success, "initialize SDL");

// Check a pointer result
SDL_Window* window = SDL_CreateWindow("Title", w, h, flags);
check_sdl(window, "create window");  // Throws if nullptr
```

### check_vma

Check VMA allocation results:

```cpp
VkResult result = vmaCreateBuffer(allocator, &bufferInfo,
                                   &allocInfo, &buffer, &allocation, nullptr);
check_vma(result, "allocate buffer");
```

## Exception Types

### VulkanException

Thrown for Vulkan API errors:

```cpp
class VulkanException : public Exception {
public:
    VulkanException(vk::Result result, std::string context,
                    std::source_location location = std::source_location::current());

    [[nodiscard]] vk::Result result() const noexcept;
};
```

Usage:

```cpp
try {
    auto buffer = allocator->allocate<VertexBuffer>(size);
} catch (const VulkanException& e) {
    std::cerr << "Vulkan error: " << e.what() << '\n';
    std::cerr << "Result: " << vk::to_string(e.result()) << '\n';
    std::cerr << "At: " << e.location().file_name()
              << ":" << e.location().line() << '\n';
}
```

### SDLException

Automatically captures SDL error message:

```cpp
class SDLException : public Exception {
public:
    SDLException(std::string context,
                 std::source_location location = std::source_location::current());
    // Constructor automatically calls SDL_GetError()
};
```

### ShaderCompilationException

Contains compilation error details:

```cpp
class ShaderCompilationException : public Exception {
public:
    ShaderCompilationException(std::string shaderName,
                               std::string errorLog,
                               std::source_location location = std::source_location::current());
};
```

Usage:

```cpp
try {
    auto shader = ShaderCompiler::compile(device, "shader.vert",
                                          vk::ShaderStageFlagBits::eVertex);
} catch (const ShaderCompilationException& e) {
    std::cerr << "Shader compilation failed:\n" << e.what() << '\n';
}
```

### LogicException

For programming errors and invalid states:

```cpp
class LogicException : public Exception {
public:
    enum class Type { out_of_range, invalid_state, null_pointer };

    LogicException(Type type, std::string message,
                   std::source_location location = std::source_location::current());
};
```

## Error Handling Patterns

### RAII Cleanup

Exceptions work naturally with RAII:

```cpp
void createResources() {
    auto buffer1 = allocator->allocate<VertexBuffer>(1000);
    auto buffer2 = allocator->allocate<IndexBuffer>(500);  // May throw

    // If buffer2 allocation throws, buffer1 is automatically freed
}
```

### Catch and Handle

```cpp
try {
    auto device = instance->findGpu()
        .requireGraphicsQueue()
        .requireRayTracingExtensions()
        .find()
        .build();
} catch (const VulkanException& e) {
    if (e.result() == vk::Result::eErrorExtensionNotPresent) {
        std::cerr << "Ray tracing not supported, falling back...\n";
        // Use fallback code path
    } else {
        throw;  // Re-throw other errors
    }
}
```

### Validation Layer Integration

Enable validation layers for detailed error messages:

```cpp
auto instance = InstanceBuilder()
    .setDebug()  // Enables validation layers
    .build();

// Validation errors will be printed to stderr
```

## Best Practices

1. **Enable validation layers** during development with `setDebug()`
2. **Catch specific exceptions** when you can handle them
3. **Let exceptions propagate** when you can't handle them
4. **Use the source location** for debugging
5. **Check error messages** - they include context about what failed
6. **Use RAII** to ensure cleanup even when exceptions occur
