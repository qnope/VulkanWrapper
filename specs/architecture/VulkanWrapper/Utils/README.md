# Utils Module

`vw` namespace · `Utils/` directory · Tier 1

Foundation utilities used by all other modules. Three key components:

## Exception Hierarchy (`Error.h`)

All exceptions derive from `vw::Exception` which captures `std::source_location` automatically. Checked via helper functions (no macros):

```cpp
check_vk(result, "context");          // throws VulkanException
check_vma(result, "context");         // throws VMAException
check_sdl(ptr_or_bool, "context");    // throws SDLException
```

| Exception | Trigger |
|-----------|---------|
| `VulkanException` | `VkResult != eSuccess` |
| `VMAException` | VMA allocation failure |
| `SDLException` | SDL API failure (captures `SDL_GetError()`) |
| `FileException` | File I/O errors |
| `AssimpException` | Model loading errors |
| `ShaderCompilationException` | GLSL→SPIR-V failure (captures compilation log) |
| `ValidationLayerException` | Vulkan debug messenger callback |
| `SwapchainException` | Recoverable swapchain errors (`needs_recreation()`, `is_out_of_date()`) |
| `LogicException` | Precondition violations (factories: `out_of_range`, `invalid_state`, `null_pointer`) |

## ObjectWithHandle\<T\> (`ObjectWithHandle.h`)

RAII handle wrapper. Template parameter controls ownership:

```cpp
ObjectWithHandle<vk::Pipeline>        // non-owning (view)
ObjectWithHandle<vk::UniquePipeline>  // owning (auto-destroyed)
// ObjectWithUniqueHandle<T> is a type alias, not a separate class
```

Range adaptor for extracting handles: `objects | vw::to_handle`

## Algos.h / Alignment.h

Algorithm and memory alignment utilities shared across modules.
