# Task 3: Triangle Example

## Summary

Implement `examples/Triangle/`, a minimal direct-rendering example that draws a single colored triangle using push constants. No deferred pipeline, no ray tracing — this is the simplest possible VulkanWrapper example.

## Dependencies

- **Task 1** (Primitive Geometry Library): Required for `create_triangle()`.

## Implementation Steps

### 1. Create `examples/Triangle/triangle.vert`

Simple vertex shader that transforms `FullVertex3D` positions with an MVP push constant:

```glsl
#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
```

Note: The input layout matches `FullVertex3D` (5 attributes: vec3, vec3, vec3, vec3, vec2). All attributes except position are unused but must be declared to match the vertex binding.

### 2. Create `examples/Triangle/triangle.frag`

Outputs a solid color from push constants:

```glsl
#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pc.color, 1.0);
}
```

### 3. Create `examples/Triangle/main.cpp`

**Push constants struct:**
```cpp
struct TrianglePushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};
```
Note: `sizeof(TrianglePushConstants)` = 76 bytes (64 + 12). Check that this is within the minimum push constant limit (128 bytes guaranteed).

**Main flow:**

1. **Setup**: Create `App` instance (Instance, Device, Swapchain, Allocator).
2. **Geometry**: Call `vw::Model::create_triangle(PlanePrimitive::XZ)` to get vertices + indices.
3. **Upload buffers**:
   - Create vertex buffer: `allocate_vertex_buffer<FullVertex3D, false>(allocator, vertices.size())`.
   - Create index buffer: `create_buffer<uint32_t, false, IndexBufferUsage>(allocator, indices.size())`.
   - Use `StagingBufferManager` to upload data via staging, submit, and wait.
4. **Compile shaders**:
   - `ShaderCompiler().compile_file_to_module(device, "triangle.vert")` (path relative to working dir).
   - `ShaderCompiler().compile_file_to_module(device, "triangle.frag")`.
5. **Create pipeline layout**:
   - `PipelineLayoutBuilder(device).with_push_constant_range({vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(TrianglePushConstants)}).build()`.
6. **Create graphics pipeline**:
   ```cpp
   auto pipeline = GraphicsPipelineBuilder(device, std::move(layout))
       .add_shader(vk::ShaderStageFlagBits::eVertex, vert_module)
       .add_shader(vk::ShaderStageFlagBits::eFragment, frag_module)
       .add_vertex_binding<FullVertex3D>()
       .add_color_attachment(swapchain_format)
       .with_dynamic_viewport_scissor()
       .build();
   ```
7. **Command pool**: `CommandPoolBuilder(device).with_reset_command_buffer().build()`, allocate buffers.
8. **Semaphores**: `imageAvailableSemaphore`, `renderFinishedSemaphore`.
9. **Render loop** (simplified, no resize handling needed for a simple example):
   - `swapchain.acquire_next_image(imageAvailableSemaphore)`
   - Record command buffer:
     - Transition swapchain image to `eColorAttachmentOptimal` via ResourceTracker
     - `cmd.beginRendering(...)` with the swapchain image view as color attachment (clear to black)
     - Set viewport and scissor dynamically
     - Bind pipeline, bind vertex buffer, bind index buffer
     - Push constants: MVP = projection * view * model (identity model, camera looking at triangle)
     - `cmd.drawIndexed(3, 1, 0, 0, 0)`
     - `cmd.endRendering()`
     - Transition swapchain image to `ePresentSrcKHR`
     - Flush ResourceTracker
   - Submit, present, wait idle
   - On frame N (e.g., 3): save screenshot and exit
10. **Screenshot**: Use `Transfer::saveToFile()` same pattern as Advanced example.

**Camera setup for the triangle:**
- The triangle is in the XZ plane (using `PlanePrimitive::XZ`), centered at origin, unit size.
- Camera: `eye = (0, 2, 0)`, looking down at `target = (0, 0, 0)`, `up = (0, 0, -1)`.
- Or simpler: use `PlanePrimitive::XY` and camera at `(0, 0, 2)` looking at origin.
- Projection: `glm::perspective(radians(60), aspect, 0.1, 10.0)` with Vulkan Y-flip.
- Color: e.g. `glm::vec3(0.2f, 0.6f, 1.0f)` (pleasant blue).

### 4. Create `examples/Triangle/CMakeLists.txt`

```cmake
add_executable(Triangle main.cpp)
target_link_libraries(Triangle PRIVATE App)
target_precompile_headers(Triangle REUSE_FROM VulkanWrapperCoreLibrary)
```

Note: Shader files (`triangle.vert`, `triangle.frag`) are loaded at runtime by `ShaderCompiler`, not compiled at build time. The working directory when running is `build/examples/Triangle/`, so the shader path in code should be relative: `"../../../examples/Triangle/triangle.vert"`.

### 5. Register in `examples/CMakeLists.txt`

Add `add_subdirectory(Triangle)` to the existing file.

## Design Considerations

- **No deferred pipeline**: This example deliberately avoids `RenderPipeline`, `RenderPass`, ray tracing. It demonstrates the raw Vulkan wrapper building blocks.
- **FullVertex3D**: Even though only position is used in the shader, we use `FullVertex3D` from the Primitive library for consistency. All attributes are declared in the shader to match the vertex binding.
- **Push constants over UBO**: Simpler for a single object. The MVP + color fit easily in push constants.
- **Shader paths**: ShaderCompiler compiles GLSL to SPIR-V at runtime. The shader files must be findable at runtime relative to the executable's working directory.

## Test Plan

- **Build validation**: `cmake --build build --target Triangle` compiles successfully.
- **Visual validation**: Run the example and inspect `screenshot.png` — should show a single colored triangle on a black background.
- **No automated tests**: This is a visual example, not a unit-testable component.
