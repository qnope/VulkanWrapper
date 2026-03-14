# Shader Module

`vw` namespace · `Shader/` directory · Tier 1

Runtime GLSL-to-SPIR-V compilation via shaderc.

## ShaderCompiler

Builder-pattern compiler with fluent configuration:

```cpp
ShaderCompiler compiler;
compiler.add_include_path(shader_dir);
auto module = compiler.compile_file_to_module(device, "sky.frag");
```

- `add_include_path(path)` — add filesystem include search path
- `add_include(name, source)` / `set_includes(map)` — virtual includes (no filesystem)
- `add_macro(name, value)` — preprocessor definitions
- `compile_file_to_module(device, path)` — file to `ShaderModule`, stage auto-detected from extension
- `compile_to_module(device, source, stage, name)` — raw source to `ShaderModule`
- `compile_from_file(path)` / `compile(source, stage, name)` — to SPIR-V bytecode
- Supported extensions: `.vert`, `.frag`, `.comp`, `.rgen`, `.rchit`, `.rmiss`, `.rahit`, `.rcall`

Compilation errors throw `ShaderCompilationException` with the full compilation log.

## Dynamic Shader Generation

Used by `DirectLightPass` and `IndirectLightPass` to generate per-material shaders at runtime. Each pass loops over `ordered_handlers()` and compiles a generated source that combines a base template with the handler's `brdf_path()`:

```cpp
auto source = "#version 460\n#include \"gbuffer_base.glsl\"\n#include \"" + handler->brdf_path() + "\"\n";
auto module = compiler.compile_to_module(device, source, eFragment, "gbuffer_dynamic.frag");
```

This eliminates static per-material shader files — adding a new material type only requires providing a BRDF snippet.
