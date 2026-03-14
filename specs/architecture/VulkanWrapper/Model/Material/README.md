# Material Module

`vw::Model::Material` namespace · `Model/Material/` directory · Tier 5

Extensible bindless material system. Each material type has its own handler that manages GPU data and provides a BRDF shader path for per-material shader generation.

## IMaterialTypeHandler (interface)

Base for all material type handlers. Constructor takes a `brdf_path` (relative path to BRDF GLSL snippet):
- `tag()` → `MaterialTypeTag` — unique type identifier
- `priority()` → `MaterialPriority` — ordering during model import (higher priority tried first)
- `try_create(aiMaterial*, base_path)` → `optional<Material>` — create from Assimp data
- `brdf_path()` → path to BRDF GLSL snippet (used for per-material shader generation)
- `buffer_address()`, `stride()` — GPU buffer info for bindless access
- `upload()` — upload material data to GPU
- `get_resources()` → `vector<ResourceState>` for barrier tracking
- `additional_descriptor_set()` / `additional_descriptor_set_layout()` — optional extra descriptors (e.g., texture arrays)

## MaterialTypeHandler\<GpuData\> (template base)

Implements `IMaterialTypeHandler` with SSBO management. Provides two material creation paths:
- `try_create(aiMaterial*)` — from Assimp import (delegates to virtual `try_create_gpu_data()`)
- `create_material(GpuData)` — programmatic creation without Assimp
- Static factory: `MaterialTypeHandler<GpuData>::create<Derived>(device, allocator, args...)`

## Built-in Handlers

| Handler | GPU Data | BRDF Shader |
|---------|----------|-------------|
| `TexturedMaterialHandler` | `{uint32_t diffuse_texture_index}` | `brdf_textured.glsl` |
| `ColoredMaterialHandler` | `{vec3 color}` | `brdf_colored.glsl` |
| `EmissiveTexturedMaterialHandler` | `{uint32_t diffuse_texture_index, float intensity}` | `brdf_emissive_textured.glsl` |

Programmatic creation: `handler.create_material(texture_path)` (textured), `handler.create_material(texture_path, intensity)` (emissive)

## BindlessMaterialManager

Manages all registered handlers:

```cpp
BindlessMaterialManager manager(device, allocator, staging);
manager.register_handler<TexturedMaterialHandler>();
manager.register_handler<ColoredMaterialHandler>();
auto material = manager.create_material(ai_mat, base_path);
manager.upload_all();
```

- `handler(tag)` — access handler by tag
- `ordered_handlers()` — deterministic tag-sorted order for SBT construction
- `sbt_mapping()` — maps `MaterialTypeTag` → SBT offset for ray tracing
- `get_resources()` — all tracked resources across handlers

## BindlessTextureManager

Manages a descriptor array of all textures for bindless access in shaders.

## Material

Lightweight struct: `MaterialTypeTag material_type` + `uint64_t buffer_address` (GPU device address for direct bindless access).

## MaterialTypeTag

Strong type for material identification. Registered via `VW_DECLARE_MATERIAL_TYPE` / `VW_DEFINE_MATERIAL_TYPE` macros. Supports hashing and ordering.
