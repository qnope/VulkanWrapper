# Examples

`examples/` directory

Six example applications demonstrating VulkanWrapper usage:

- [Application](Application/README.md) — Basic windowed Vulkan application setup (`App` class)
- **Common** — Shared `ExampleRunner` base class that handles frame loop boilerplate (command pool, swapchain acquire/present, blit, screenshot, resize, error handling)
- **Triangle** — Minimal rasterization example (manual dynamic rendering to swapchain)
- **CubeShadow** — Ray-traced cube with shadow, sky, and tone mapping (ZPass → DirectLight → Sky → ToneMapping)
- **AmbientOcclusion** — Ray-traced ambient occlusion (ZPass → DirectLight → AO), screenshots at 256 samples
- **EmissiveCube** — Full pipeline with emissive materials (ZPass → DirectLight → AO → Sky → IndirectLight → ToneMapping)
- [Advanced](Advanced/README.md) — Full deferred rendering pipeline with ray tracing

All examples use the `App` class from Application as the base setup (instance, device, allocator, window, swapchain). Triangle, CubeShadow, AmbientOcclusion, and EmissiveCube extend `ExampleRunner` to share the frame loop, while Advanced has its own loop.
