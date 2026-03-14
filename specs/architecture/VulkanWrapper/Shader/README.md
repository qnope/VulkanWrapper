# Shader Module

`vw` namespace · `Shader/` directory · Tier 1

Runtime GLSL-to-SPIR-V compilation via shaderc.

## ShaderCompiler

- `compile_file_to_module(device, "file.frag")` — detects stage from file extension
- Supports `#include` directives via include resolver
- Used by all render passes (SkyPass, ToneMappingPass, IndirectLightPass) to compile shaders at construction time
- IndirectLightPass dynamically generates per-material closest hit shaders by combining `indirect_light_base.glsl` with handler `brdf_path()`

```cpp
ShaderCompiler compiler;
auto module = compiler.compile_file_to_module(device, "sky.frag");
```

Compilation errors throw `ShaderCompilationException` with the full compilation log.
