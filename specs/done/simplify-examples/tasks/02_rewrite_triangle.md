# Task 2: Rewrite Triangle Example

## Summary

Rewrite `examples/Triangle/main.cpp` to use `ExampleRunner`. The Triangle example is unique: it renders directly to the swapchain image using manual dynamic rendering (no `RenderPipeline`). It does not need `result_image()` (renders to swapchain), uses the default screenshot at frame 3, and does not need resize handling beyond the base class default.

## Target: ~80 lines

## Implementation Steps

### 1. Create `TriangleExample` class

```cpp
class TriangleExample : public ExampleRunner {
    // Members (created in setup()):
    // - vertex_buffer, index_buffer
    // - pipeline (GraphicsPipeline)
    // - push_constants (TrianglePushConstants)
    // - index_count (uint32_t, for drawIndexed)

    void setup() override;
    void render(vw::CommandBuffer &cmd,
                vw::Transfer &transfer,
                uint32_t frame_index) override;
    // result_image() → default nullptr (renders to swapchain)
    // should_screenshot() → default (frame 3)
    // on_resize() → default (no-op, camera is static)
};
```

### 2. `setup()` implementation

Move from current main():
- Create triangle geometry via `vw::Model::create_triangle(XY)`
- Upload vertex/index buffers via `StagingBufferManager`
- Compile vertex + fragment shaders
- Build pipeline layout with push constants
- Build graphics pipeline with vertex binding, color attachment, dynamic viewport/scissor
- Compute camera MVP matrix (static, does not change on resize since aspect is baked in at setup time — acceptable for a simple demo)

### 3. `render()` implementation

Move the rendering code from the current frame loop:
- Transition swapchain image to `eColorAttachmentOptimal` (Triangle-specific, not in base class)
- Flush tracker
- Begin dynamic rendering with the swapchain image view
- Set viewport/scissor
- Bind pipeline, vertex buffer, index buffer
- Push constants
- Draw indexed
- End rendering

**Important**: Triangle renders to the swapchain directly, so it must transition the swapchain image to `eColorAttachmentOptimal` itself inside `render()`. The base class will handle the final transition to `ePresentSrcKHR`.

### 4. `main()` becomes minimal

```cpp
int main() {
    App app;
    TriangleExample example(app);
    example.run();
}
```

The error handling `try/catch` is inside `ExampleRunner::run()`.

### 5. Access to swapchain image in `render()`

Triangle needs the swapchain image view for dynamic rendering. It gets the image view from `app().swapchain.image_views()[frame_index]`.

## Files to Modify

### `examples/Triangle/main.cpp`
- Complete rewrite: replace 309-line main() with ~80-line TriangleExample class + minimal main()

### `examples/Triangle/CMakeLists.txt`
- Add `ExamplesCommon` dependency:
```cmake
add_executable(Triangle main.cpp)
target_link_libraries(Triangle PRIVATE ExamplesCommon)
target_precompile_headers(Triangle REUSE_FROM VulkanWrapperCoreLibrary)
```
Note: `ExamplesCommon` now transitively provides `App` (PUBLIC dependency), so `App` can be removed from the link line.

## Dependencies

- Task 1 (ExampleRunner base class)

## Test Plan

1. Build: `cmake --build build --target Triangle`
2. Run: `cd build/examples/Triangle && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./Triangle`
3. Verify `screenshot.png` is produced
4. Compare screenshot with pre-refactor version (should be pixel-identical)
5. Verify it screenshots at frame 3
