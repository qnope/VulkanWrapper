# Task 1: Create ExampleRunner Base Class

## Summary

Create the `ExampleRunner` base class in `examples/Common/` that encapsulates the shared frame loop boilerplate: command pool/semaphore creation, swapchain acquire/present, command buffer recording, result-to-swapchain blit, screenshot capture, swapchain recreation, and error handling. Update the Common CMakeLists.txt from INTERFACE to STATIC library.

## Files to Create

### `examples/Common/ExampleRunner.h`

```cpp
#pragma once

#include "Application.h"
#include <VulkanWrapper/Memory/Transfer.h>
// forward declarations as needed
```

**Class definition:**
```cpp
class ExampleRunner {
public:
    explicit ExampleRunner(App &app);
    virtual ~ExampleRunner() = default;

    void run();

protected:
    // --- Pure virtual hooks ---
    virtual void setup() = 0;
    virtual void render(vw::CommandBuffer &cmd,
                        vw::Transfer &transfer,
                        uint32_t frame_index) = 0;

    // --- Virtual hooks with defaults ---
    virtual std::shared_ptr<const vw::ImageView> result_image();
    virtual void on_resize(vw::Width width, vw::Height height);
    virtual bool should_screenshot() const;

    // --- Utilities for derived classes ---
    App &app();
    vw::Transfer &transfer();
    int frame_count() const;

private:
    App &m_app;
    vw::Transfer m_transfer;
    int m_frame_count = 0;

    // Synchronization & command recording (created in run())
    // These are implementation details, not exposed to subclasses
};
```

### `examples/Common/ExampleRunner.cpp`

**Implementation of `run()`:**

```
run() pseudocode:
1. Call setup()
2. Create CommandPool (with_reset_command_buffer)
3. Allocate command buffers (one per swapchain image)
4. Create two semaphores (imageAvailable, renderFinished)
5. Enter main loop: while (!window.is_close_requested())
   a. window.update()
   b. if (window.is_resized()) → call handle_resize(), continue
   c. try:
      - acquire_next_image(imageAvailableSemaphore) → index
      - Reset and begin recording commandBuffers[index]
      - Call render(cmd, transfer, frame_index)
      - If result_image() != nullptr:
        - Blit result_image to swapchain image
      - Transition swapchain image to ePresentSrcKHR
      - Flush resource tracker
      - End recording
      - Submit (wait on imageAvailable, signal renderFinished)
      - Present
      - wait_idle()
      - Print frame info (cout << "Iteration: " << m_frame_count)
      - m_frame_count++
      - If should_screenshot():
        - Reset cmd, begin recording
        - Transition swapchain to eTransferSrcOptimal
        - Flush tracker
        - saveToFile → "screenshot.png"
        - Print message
        - Break
   d. catch (SwapchainException) → call handle_resize()
6. wait_idle()
```

**Private `handle_resize()` method:**
```
- Get window width/height; if zero, return
- wait_idle()
- Rebuild swapchain with old swapchain
- Reallocate command buffers
- Call on_resize(width, height) (virtual, for subclass logic)
- Reset frame count to 0
```

**Default implementations:**
- `result_image()` → return `nullptr`
- `on_resize(w, h)` → no-op (subclasses override for pipeline reset, UBO update)
- `should_screenshot()` → return `m_frame_count == 3`
- `app()` → return `m_app`
- `transfer()` → return `m_transfer`
- `frame_count()` → return `m_frame_count`

**Error handling:**
The outer `try/catch` for `vw::Exception` and `std::exception` is in `run()`.

### Key design decisions

1. **Transition to color attachment for Triangle**: Triangle renders directly to swapchain, which requires transitioning the swapchain image to `eColorAttachmentOptimal` before render(). For RT examples that use RenderPipeline, the swapchain image is never used as a color attachment (only as a blit target). Solution: the transition to `eColorAttachmentOptimal` should NOT be in the base class. Triangle will handle it inside its own `render()` override. The base class only handles the present transition and blit.

2. **No transition to color attachment in base class**: The base class's command buffer recording scope only:
   - Calls `render(cmd, transfer, frame_index)` — subclass does whatever transitions it needs
   - If `result_image()` is non-null, calls `transfer.blit()` to swapchain
   - Transitions swapchain to `ePresentSrcKHR`
   - Flushes tracker

3. **Transfer ownership**: The base class owns the `vw::Transfer` instance and passes it to `render()`. Subclasses can also track additional resource states via `transfer().resourceTracker().track()`.

4. **Frame count**: The base class tracks frame count for `should_screenshot()`. Subclasses that need different screenshot logic (e.g., AO sample count) override `should_screenshot()`.

## Files to Modify

### `examples/Common/CMakeLists.txt`

Change from INTERFACE to STATIC library:
```cmake
add_library(ExamplesCommon STATIC ExampleRunner.cpp ExampleRunner.h SceneSetup.h)
target_include_directories(ExamplesCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ExamplesCommon PUBLIC VulkanWrapper::VW App)
target_precompile_headers(ExamplesCommon REUSE_FROM VulkanWrapperCoreLibrary)
```

Key changes:
- `INTERFACE` → `STATIC`
- Add `ExampleRunner.cpp`, `ExampleRunner.h`, `SceneSetup.h` as sources
- `INTERFACE` → `PUBLIC` for include dirs and link libraries
- Add `App` as PUBLIC dependency (ExampleRunner.h includes Application.h)
- Add PCH reuse

## Includes needed in ExampleRunner.cpp

```cpp
#include "ExampleRunner.h"
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Vulkan/Queue.h>
```

## Dependencies

- `App` class (examples/Application/)
- VulkanWrapper library (Command, Memory, Synchronization, Vulkan modules)

## Test Plan

- Build verification: `cmake --build build --target ExamplesCommon` must succeed
- ExampleRunner cannot be tested in isolation (abstract class) — validated through tasks 2-5
