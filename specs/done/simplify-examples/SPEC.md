# Simplify Examples — API Specification

## 1. Feature Overview

The four standalone examples (Triangle, CubeShadow, AmbientOcclusion, EmissiveCube) share ~80% identical boilerplate: frame loop, swapchain recreation, command buffer submit, screenshot capture, present transition, and error handling. This project extracts that boilerplate into a reusable `ExampleRunner` base class in `examples/Common/`, so each example only defines its scene, render passes, and per-frame configuration.

**Goal:** reduce each example's `main.cpp` to ~100–150 lines while keeping the rendering flow readable. The frame loop stays visible but the mechanical parts (submit, blit, present, resize, screenshot, error handling) are handled by the base class.

## 2. API Design

### 2.1 ExampleRunner base class (`examples/Common/ExampleRunner.h`)

A base class that owns the full frame loop and delegates rendering decisions to virtual methods.

```cpp
// examples/Common/ExampleRunner.h
#pragma once

class ExampleRunner {
  public:
    explicit ExampleRunner(App &app);
    virtual ~ExampleRunner() = default;

    // Run the example until window close or screenshot taken.
    // Handles: window events, swapchain recreation, command buffer
    // recording, submit, present, wait_idle, error handling.
    void run();

  protected:
    // --- Virtual hooks (override in derived classes) ---

    // Called once before the frame loop starts.
    // Use to create pipelines, upload meshes, etc.
    virtual void setup() = 0;

    // Called each frame inside the command buffer recording scope.
    // The command buffer is already being recorded.
    // `frame_index` is the swapchain image index.
    virtual void render(vw::CommandBuffer &cmd,
                        vw::Transfer &transfer,
                        uint32_t frame_index) = 0;

    // Return the image to blit to the swapchain after render().
    // Default: returns nullptr (no blit — example rendered directly
    // to swapchain, like Triangle).
    virtual std::shared_ptr<const vw::ImageView> result_image();

    // Called when the window is resized. Default implementation
    // rebuilds swapchain, reallocates command buffers. Override to
    // add pass-specific reset (e.g. pipeline.reset_accumulation(),
    // UBO update).
    virtual void on_resize(vw::Width width, vw::Height height);

    // Return true when the example should take a screenshot and exit.
    // Called after each frame. Default: screenshot at frame 3.
    virtual bool should_screenshot() const;

    // --- Utilities available to derived classes ---
    App &app();
    vw::Transfer &transfer();
    int frame_count() const;
};
```

### 2.2 How the frame loop works (inside `run()`)

The base class `run()` method implements the shared frame loop:

1. Calls `setup()` once.
2. Enters the window event loop.
3. On resize: calls `on_resize()`.
4. Each frame:
   a. Acquires swapchain image.
   b. Resets and begins recording the command buffer.
   c. Calls `render(cmd, transfer, frame_index)`.
   d. If `result_image()` returns non-null, blits it to the swapchain.
   e. Transitions swapchain image to present layout.
   f. Ends recording, submits, presents, waits idle.
   g. If `should_screenshot()` returns true, captures screenshot and breaks.
5. On `SwapchainException`: calls `on_resize()`.
6. Wraps everything in the standard `vw::Exception` / `std::exception` error handler.

### 2.3 How each example uses it

#### Triangle (~80 lines)

```cpp
class TriangleExample : public ExampleRunner {
    // Owns: vertex/index buffers, pipeline, push constants
    void setup() override;        // Upload geometry, compile shaders, build pipeline
    void render(...) override;    // Manual dynamic rendering (beginRendering/endRendering)
    // result_image() → default nullptr (renders directly to swapchain)
    // should_screenshot() → default (frame 3)
};
```

Triangle does **not** use `RenderPipeline`. It overrides `render()` entirely to do manual dynamic rendering directly to the swapchain image.

#### CubeShadow (~100 lines)

```cpp
class CubeShadowExample : public ExampleRunner {
    // Owns: mesh_manager, rayTracedScene, pipeline (ZPass, DirectLight, Sky, ToneMapping),
    //       uniform_buffer, sky_params
    void setup() override;        // Create scene, build RT, create passes
    void render(...) override;    // Configure passes, execute pipeline
    std::shared_ptr<const vw::ImageView> result_image() override;  // ToneMapped slot
    void on_resize(...) override; // Reset accumulation + update UBO
    // should_screenshot() → default (frame 3)
};
```

#### AmbientOcclusion (~100 lines)

```cpp
class AOExample : public ExampleRunner {
    // Owns: mesh_manager, rayTracedScene, pipeline (ZPass, DirectLight, AO),
    //       uniform_buffer
    void setup() override;
    void render(...) override;
    std::shared_ptr<const vw::ImageView> result_image() override;  // AmbientOcclusion slot
    void on_resize(...) override;
    bool should_screenshot() const override;  // aoPass.get_frame_count() == 256
};
```

#### EmissiveCube (~120 lines)

```cpp
class EmissiveCubeExample : public ExampleRunner {
    // Owns: mesh_manager, rayTracedScene, full pipeline (Z, Direct, AO, Sky, Indirect, ToneMapping),
    //       uniform_buffer, emissive material
    void setup() override;
    void render(...) override;
    std::shared_ptr<const vw::ImageView> result_image() override;  // ToneMapped slot
    void on_resize(...) override;
    bool should_screenshot() const override;  // aoPass.get_frame_count() == 256
};
```

### 2.4 What gets factorized into `ExampleRunner::run()`

| Boilerplate | Current location | Lines saved per example |
|---|---|---|
| Command pool + semaphore creation | Each main() | ~8 |
| Frame loop structure (acquire, record, submit, present, wait_idle) | Each main() | ~30 |
| Blit result to swapchain + present transition | CubeShadow, AO, Emissive | ~15 |
| Screenshot capture (transition + saveToFile) | All four | ~20 |
| Swapchain recreation (rebuild + reallocate cmd buffers) | All four | ~15 |
| Error handling (outer try/catch) | All four | ~10 |
| **Total** | | **~80–100 lines per example** |

### 2.5 What stays in each example

- Scene creation (meshes, materials, instances)
- Render pass construction and pipeline assembly
- Per-frame pass configuration (set_uniform_buffer, set_sky_parameters, etc.)
- Pass-specific resize logic (reset_accumulation, UBO update)
- Screenshot condition logic

### 2.6 File changes

| File | Action |
|---|---|
| `examples/Common/ExampleRunner.h` | **New** — base class definition |
| `examples/Common/ExampleRunner.cpp` | **New** — `run()` implementation |
| `examples/Common/CMakeLists.txt` | **Modify** — change from INTERFACE to STATIC library, add ExampleRunner.cpp |
| `examples/Triangle/main.cpp` | **Rewrite** — TriangleExample class + minimal main() |
| `examples/CubeShadow/main.cpp` | **Rewrite** — CubeShadowExample class + minimal main() |
| `examples/AmbientOcclusion/main.cpp` | **Rewrite** — AOExample class + minimal main() |
| `examples/EmissiveCube/main.cpp` | **Rewrite** — EmissiveCubeExample class + minimal main() |
| `examples/Triangle/CMakeLists.txt` | **Modify** — link ExamplesCommon (currently only links App) |

## 3. Testing and Validation

### 3.1 Build verification
- All five example targets (Triangle, CubeShadow, AmbientOcclusion, EmissiveCube, Advanced) must compile and link successfully.

### 3.2 Screenshot regression
- Run each of the four examples and verify that `screenshot.png` is produced.
- Compare output screenshots before/after the refactor to ensure pixel-identical (or near-identical) results. The refactoring must not change any rendering behavior.

### 3.3 Behavioral validation
- CubeShadow screenshots at frame 3.
- AmbientOcclusion and EmissiveCube screenshot at 256 AO samples.
- Triangle screenshots at frame 3.
- Window resize handling still works (swapchain recreation, UBO update, accumulation reset).

### 3.4 Success criteria
- Each example's `main.cpp` is between 80–150 lines (down from 240–320).
- No rendering behavior changes.
- No new library dependencies introduced (ExampleRunner is example-only code).
- The Advanced example is **not** affected by this change.
