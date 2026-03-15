---
sidebar_position: 3
title: "Shaders"
---

# Shaders

## Overview

VulkanWrapper provides runtime GLSL-to-SPIR-V compilation through the
`ShaderCompiler` class and a thin `ShaderModule` RAII wrapper.  This means you
can write your shaders in plain GLSL, and the library will compile them at
application startup -- no offline compilation step needed.

Key capabilities:

- All Vulkan shader stages: vertex, fragment, compute, geometry, tessellation,
  and the full set of ray tracing stages.
- `#include` directive support with configurable search paths and virtual
  (in-memory) files.
- Preprocessor macro injection.
- Automatic stage detection from file extensions.
- Compilation error messages with source locations.

### Where it lives in the library

| Item | Header |
|------|--------|
| `ShaderCompiler` | `VulkanWrapper/Shader/ShaderCompiler.h` |
| `ShaderModule` | `VulkanWrapper/Pipeline/ShaderModule.h` |
| `ShaderCompilationResult` | `VulkanWrapper/Shader/ShaderCompiler.h` |
| `IncludeMap` | `VulkanWrapper/Shader/ShaderCompiler.h` |

All types live in the `vw` namespace.

---

## API Reference

### ShaderCompiler

`ShaderCompiler` is non-copyable but movable.  It uses the pImpl pattern
internally (backed by shaderc).

#### Configuration (fluent, chainable)

| Method | Returns | Description |
|--------|---------|-------------|
| `ShaderCompiler()` | -- | Default constructor. |
| `add_include_path(const std::filesystem::path& path)` | `ShaderCompiler&` | Add a directory to the include search path. Searched in order when resolving `#include "..."` directives. |
| `add_include(std::string_view name, std::string_view content)` | `ShaderCompiler&` | Register a virtual include file. When the compiler encounters `#include "name"`, it uses `content` directly instead of reading from disk. |
| `set_includes(IncludeMap includes)` | `ShaderCompiler&` | Replace all virtual includes at once. `IncludeMap` is `std::map<std::string, std::string>`. |
| `add_macro(std::string_view name, std::string_view value = "")` | `ShaderCompiler&` | Add a preprocessor macro definition. Equivalent to `#define name value` at the top of every compiled shader. |
| `set_target_vulkan_version(uint32_t version)` | `ShaderCompiler&` | Set the target Vulkan API version (e.g., `VK_API_VERSION_1_3`). Affects the SPIR-V version emitted. |
| `set_generate_debug_info(bool enable)` | `ShaderCompiler&` | Enable debug info in the SPIR-V output (useful for shader debuggers). |
| `set_optimize(bool enable)` | `ShaderCompiler&` | Enable SPIR-V optimization (reduces binary size, may improve GPU performance). |

#### Compilation

| Method | Returns | Description |
|--------|---------|-------------|
| `compile(source, stage, sourceName = "<source>")` | `ShaderCompilationResult` | Compile GLSL source string to SPIR-V. Throws `ShaderCompilationException` on failure. |
| `compile_from_file(path)` | `ShaderCompilationResult` | Compile a file. Stage is auto-detected from the extension. Throws `FileException` if the file cannot be read. |
| `compile_from_file(path, stage)` | `ShaderCompilationResult` | Compile a file with an explicit stage (overrides extension detection). |
| `compile_to_module(device, source, stage, sourceName = "<source>")` | `std::shared_ptr<const ShaderModule>` | Compile GLSL and create a `ShaderModule` in one step. |
| `compile_file_to_module(device, path)` | `std::shared_ptr<const ShaderModule>` | Compile a file and create a `ShaderModule` in one step. Stage is auto-detected. |

#### Utilities

| Method | Returns | Description |
|--------|---------|-------------|
| `static detect_stage_from_extension(path)` | `vk::ShaderStageFlagBits` | Detect the shader stage from a file path. Throws `ShaderCompilationException` if the extension is not recognized. |

**Supported extensions and stages:**

| Extension | Stage | Extension | Stage |
|-----------|-------|-----------|-------|
| `.vert` | `eVertex` | `.rgen` | `eRaygenKHR` |
| `.frag` | `eFragment` | `.rchit` | `eClosestHitKHR` |
| `.comp` | `eCompute` | `.rmiss` | `eMissKHR` |
| `.geom` | `eGeometry` | `.rahit` | `eAnyHitKHR` |
| `.tesc` | `eTessellationControl` | `.rcall` | `eCallableKHR` |
| `.tese` | `eTessellationEvaluation` | | |

Double extensions like `.vert.glsl` and `.frag.glsl` are also supported -- the
stage is detected from the second-to-last extension.

### ShaderCompilationResult

```cpp
struct ShaderCompilationResult {
    std::vector<std::uint32_t> spirv;              // SPIR-V bytecode
    std::set<std::filesystem::path> includedFiles; // All files that were read
};
```

The `includedFiles` set contains every file that contributed to the compilation
(the main file plus all `#include`d files).  This is useful for hot-reload
systems that need to watch dependencies.

### ShaderModule

`ShaderModule` inherits `ObjectWithUniqueHandle<vk::UniqueShaderModule>`.

| Method | Returns | Description |
|--------|---------|-------------|
| `handle()` | `vk::ShaderModule` | The raw Vulkan handle (inherited). |
| `static create_from_spirv(device, std::span<const uint32_t> spirv)` | `std::shared_ptr<const ShaderModule>` | Create a module from pre-compiled SPIR-V bytes. |
| `static create_from_spirv_file(device, path)` | `std::shared_ptr<const ShaderModule>` | Load a `.spv` file from disk and create a module. |

---

## Usage Examples

### Compile a file and create a module

```cpp
vw::ShaderCompiler compiler;
auto vertModule = compiler.compile_file_to_module(device, "shaders/mesh.vert");
auto fragModule = compiler.compile_file_to_module(device, "shaders/mesh.frag");
```

### Compile inline GLSL

```cpp
const std::string fragSource = R"(
#version 450
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

vw::ShaderCompiler compiler;
auto fragModule = compiler.compile_to_module(
    device, fragSource, vk::ShaderStageFlagBits::eFragment);
```

### Include paths and virtual includes

```cpp
vw::ShaderCompiler compiler;

// Search for #include files in this directory
compiler.add_include_path("Shaders/include");

// Register a virtual include (not on disk)
compiler.add_include("config.glsl", "#define MAX_LIGHTS 16\n");

auto result = compiler.compile_from_file("Shaders/lighting.frag");
```

### Using set_includes for multiple virtual files

```cpp
vw::IncludeMap includes;
includes["constants.glsl"] = "#define PI 3.14159265359\n";
includes["utils.glsl"]     = "#define GREEN 0.5\n";

vw::ShaderCompiler compiler;
compiler.set_includes(std::move(includes));

auto result = compiler.compile(shaderSource,
                               vk::ShaderStageFlagBits::eFragment);
```

### Preprocessor macros

```cpp
vw::ShaderCompiler compiler;
compiler.add_macro("ENABLE_SHADOWS");
compiler.add_macro("MAX_SAMPLES", "64");

auto module = compiler.compile_file_to_module(device, "shaders/main.frag");
```

### Full configuration chain

```cpp
auto result =
    vw::ShaderCompiler()
        .add_include_path("Shaders/include")
        .set_target_vulkan_version(VK_API_VERSION_1_3)
        .add_macro("VULKAN_1_3")
        .set_generate_debug_info(true)
        .set_optimize(false)
        .compile_from_file("Shaders/deferred.frag");
```

### Loading pre-compiled SPIR-V

If you prefer offline compilation (e.g., via `glslangValidator` or
`glslc`), you can load `.spv` files directly:

```cpp
auto module = vw::ShaderModule::create_from_spirv_file(
    device, "shaders/mesh.vert.spv");
```

Or from an in-memory SPIR-V vector:

```cpp
std::vector<uint32_t> spirvData = /* loaded from file or embedded */;
auto module = vw::ShaderModule::create_from_spirv(device, spirvData);
```

---

## Integration Patterns

### Shader to Pipeline flow

```
ShaderCompiler  --->  ShaderModule  --->  GraphicsPipelineBuilder
  (GLSL)              (SPIR-V)            (GPU state)
```

Typical code:

```cpp
vw::ShaderCompiler compiler;
compiler.add_include_path("Shaders/include");

auto vert = compiler.compile_file_to_module(device, "Shaders/mesh.vert");
auto frag = compiler.compile_file_to_module(device, "Shaders/mesh.frag");

auto pipeline =
    vw::GraphicsPipelineBuilder(device, layout)
        .add_shader(vk::ShaderStageFlagBits::eVertex, vert)
        .add_shader(vk::ShaderStageFlagBits::eFragment, frag)
        // ... other state ...
        .build();
```

### Hot-reload using includedFiles

```cpp
auto result = compiler.compile_from_file("shaders/main.frag");

// Watch all dependency files for changes
for (const auto& dep : result.includedFiles) {
    fileWatcher.watch(dep, [&] { recompile(); });
}
```

### Screen-space pass shortcut

For fullscreen post-processing passes, `create_screen_space_pipeline()` wires
the vertex and fragment shaders into a ready-to-use pipeline:

```cpp
auto vert = compiler.compile_to_module(
    device, fullscreenVertSrc, vk::ShaderStageFlagBits::eVertex);
auto frag = compiler.compile_to_module(
    device, tonemapFragSrc, vk::ShaderStageFlagBits::eFragment);

auto pipeline = vw::create_screen_space_pipeline(
    device, vert, frag, descriptorLayout,
    vk::Format::eR8G8B8A8Unorm);
```

---

## Error Handling

The compiler throws typed exceptions from the `vw::Exception` hierarchy:

| Exception | When |
|-----------|------|
| `ShaderCompilationException` | GLSL syntax error, unresolved include, unsupported extension |
| `FileException` | Source file does not exist or cannot be read |

`ShaderCompilationException` provides additional context:

```cpp
try {
    compiler.compile(badGlsl, vk::ShaderStageFlagBits::eVertex);
} catch (const vw::ShaderCompilationException& e) {
    std::cerr << "Stage: " << vk::to_string(e.stage()) << "\n";
    std::cerr << "Log:\n" << e.compilation_log() << "\n";
}
```

---

## Common Pitfalls

1. **Missing `#version` directive.**
   GLSL files must start with `#version 450` (or later).  Without it, shaderc
   defaults to an older GLSL version and most Vulkan features will not compile.

2. **Include path not set.**
   If your shader uses `#include "foo.glsl"` but you did not call
   `add_include_path()` with the directory containing `foo.glsl`, compilation
   will throw `ShaderCompilationException`.

3. **Extension mismatch.**
   `compile_from_file()` (single-argument) detects the stage from the
   extension.  If your file is named `shader.glsl` (ambiguous), you must use
   the overload that takes an explicit `vk::ShaderStageFlagBits` stage, or
   rename the file to use a recognized extension.

4. **Forgetting to pass the `Device` for module creation.**
   `compile()` and `compile_from_file()` return raw SPIR-V bytes, not a
   `ShaderModule`.  To feed the result into `GraphicsPipelineBuilder`, use
   `compile_to_module()` or `compile_file_to_module()` which create the
   `VkShaderModule` handle for you.

5. **Keeping the compiler alive unnecessarily.**
   `ShaderModule` objects are independent of the `ShaderCompiler` that created
   them.  You can destroy the compiler as soon as compilation is done.

6. **Stage mismatch in `add_shader()`.**
   The `vk::ShaderStageFlagBits` passed to
   `GraphicsPipelineBuilder::add_shader()` must match the actual stage the
   shader was compiled for.  If you compile as `eVertex` but add as
   `eFragment`, you will get driver errors.
