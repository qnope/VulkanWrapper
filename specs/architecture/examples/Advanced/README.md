# Advanced Example

`examples/Advanced/` directory

Full deferred rendering pipeline with physically-based lighting, ambient occlusion, and ray-traced indirect illumination. Renders the Sponza scene with an emissive stained-glass quad and saves a screenshot after 32 AO samples.

## Pipeline Flow

Managed by `vw::RenderPipeline` (library class):

```
ZPass → DirectLightPass → AO Pass → SkyPass → IndirectLightPass → ToneMappingPass → Swapchain
```

All passes are library-provided. The example constructs a `RenderPipeline`, adds passes, validates slot dependencies, and executes per frame:

```cpp
vw::RenderPipeline pipeline;
auto& zpass = pipeline.add(std::make_unique<vw::ZPass>(device, allocator, shader_dir));
auto& directLight = pipeline.add(std::make_unique<vw::DirectLightPass>(...));
// ... more passes
pipeline.validate();
pipeline.execute(cmd, tracker, width, height, frame_index);
```

## Key Files

| File | Purpose |
|------|---------|
| `main.cpp` | Pipeline construction, render loop, per-frame pass configuration, screenshot capture |
| `SceneSetup.h` | Sponza scene loading + emissive stained-glass quad creation |
| `RenderPassInformation.h` | UBO and camera structures |

## Running

`cd build/examples/Advanced && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./Main`

Outputs `screenshot.png` after 32 progressive AO samples.
