# Material Module

`vw::Model::Material` namespace · `Model/Material/` directory · Tier 5

Extensible bindless material system. Each material type has its own handler that manages GPU data and provides a BRDF shader path for per-material shader generation.

## IMaterialTypeHandler (interface)

Base for all material type handlers:
- `tag()` → `MaterialTypeTag` — unique type identifier
- `priority()` → `MaterialPriority` — ordering during model import (higher priority handlers are tried first)
- `try_create(aiMaterial*, base_path)` → `optional<Material>` — attempt to create material from Assimp data
- `brdf_path()` → path to BRDF GLSL snippet (used for per-material shader generation)
- `buffer_address()`, `stride()` — GPU buffer info for bindless access
- `upload()` — upload material data to GPU
- `get_resources()` → `vector<ResourceState>` for barrier tracking
- `additional_descriptor_set()` — optional extra descriptors (e.g., texture arrays)

## Built-in Handlers

| Handler | Description | BRDF Shader |
|---------|-------------|-------------|
| `TexturedMaterialHandler` | Diffuse-textured materials | `brdf_textured.glsl` |
| `ColoredMaterialHandler` | Solid-color materials | `brdf_colored.glsl` |
| `EmissiveTexturedMaterialHandler` | Emissive textured materials | `brdf_emissive_textured.glsl` |

Each handler generates per-material closest hit shaders by combining `indirect_light_base.glsl` with its `brdf_path()`.

## BindlessMaterialManager

Manages all registered handlers:

```cpp
BindlessMaterialManager manager(device, allocator, staging);
manager.register_handler<TexturedMaterialHandler>();
manager.register_handler<ColoredMaterialHandler>();
auto material = manager.create_material(ai_mat, base_path);
manager.upload_all();
```

- `sbt_mapping()` — maps `MaterialTypeTag` → SBT offset for ray tracing
- `ordered_handlers()` — deterministic ordering for SBT construction
- `get_resources()` — all tracked resources across handlers

## BindlessTextureManager

Manages a descriptor array of all textures for bindless access in shaders.

## Material

Lightweight struct (`tag` + material index) identifying a mesh's material.

## MaterialTypeTag

Strong type for material identification. Used as key in maps, supports hashing. Registered via `VW_DECLARE_MATERIAL_TYPE` / `VW_DEFINE_MATERIAL_TYPE` macros.
