# Sequence Diagram Templates

## Template 1: Initialization Sequence

Use for documenting startup flows.

```mermaid
sequenceDiagram
    participant App
    participant Instance
    participant DeviceFinder
    participant Device
    participant Allocator

    App->>Instance: InstanceBuilder().build()
    activate Instance
    Instance->>Instance: Create VkInstance
    Instance->>Instance: Setup debug utils
    Instance-->>App: shared_ptr<Instance>
    deactivate Instance

    App->>DeviceFinder: instance->findGpu()
    activate DeviceFinder
    DeviceFinder->>DeviceFinder: Enumerate physical devices
    DeviceFinder->>DeviceFinder: Check requirements
    App->>DeviceFinder: with_queue(Graphics)
    App->>DeviceFinder: with_dynamic_rendering()
    App->>DeviceFinder: build()
    DeviceFinder->>Device: Create logical device
    activate Device
    Device-->>App: shared_ptr<Device>
    deactivate Device
    deactivate DeviceFinder

    App->>Allocator: AllocatorBuilder().build()
    activate Allocator
    Allocator->>Allocator: Create VMA allocator
    Allocator-->>App: shared_ptr<Allocator>
    deactivate Allocator
```

## Template 2: Render Frame Sequence

Use for documenting frame rendering.

```mermaid
sequenceDiagram
    participant App
    participant Swapchain
    participant CommandBuffer
    participant Queue
    participant GPU

    App->>Swapchain: acquire_next_image()
    Swapchain-->>App: imageIndex, semaphore

    App->>CommandBuffer: begin()
    App->>CommandBuffer: Record barriers
    App->>CommandBuffer: beginRendering()
    App->>CommandBuffer: bindPipeline()
    App->>CommandBuffer: draw()
    App->>CommandBuffer: endRendering()
    App->>CommandBuffer: end()

    App->>Queue: submit2(cmd, fence)
    Queue->>GPU: Execute commands
    GPU-->>Queue: Signal fence

    App->>Swapchain: present(imageIndex)
```

## Template 3: Resource Upload Sequence

Use for documenting staging buffer uploads.

```mermaid
sequenceDiagram
    participant App
    participant Staging as StagingBufferManager
    participant CommandBuffer
    participant GPU
    participant Image

    App->>Staging: stage_image(image, pixels)
    Note over Staging: Allocate staging buffer
    Note over Staging: Copy pixels to staging

    App->>CommandBuffer: begin()
    App->>Staging: flush(cmd)
    Note over Staging: Record vkCmdCopyBufferToImage
    App->>CommandBuffer: end()

    App->>GPU: Submit and wait
    GPU->>Image: Copy data
    GPU-->>App: Complete

    Note over Image: Data now on GPU
```

## Template 4: Descriptor Update Sequence

Use for documenting descriptor binding.

```mermaid
sequenceDiagram
    participant App
    participant Layout as DescriptorSetLayout
    participant Pool as DescriptorPool
    participant Set as DescriptorSet
    participant Pipeline

    App->>Layout: DescriptorSetLayoutBuilder().build()
    Layout-->>App: shared_ptr<Layout>

    App->>Pool: Create with pool sizes
    Pool-->>App: unique_ptr<Pool>

    App->>Pool: allocate(layout)
    Pool->>Set: Create VkDescriptorSet
    Set-->>App: DescriptorSet

    App->>Set: update_combined_image_sampler(0, sampler, view)
    Note over Set: vkUpdateDescriptorSets

    Note over App: During rendering:
    App->>Pipeline: bindDescriptorSets(set)
```

## Template 5: Ray Tracing Build Sequence

Use for documenting acceleration structure building.

```mermaid
sequenceDiagram
    participant App
    participant BLAS
    participant TLAS
    participant CommandBuffer
    participant GPU

    Note over App: For each mesh:
    App->>BLAS: Create BLAS builder
    App->>BLAS: add_triangles(vertices, indices)
    App->>BLAS: build()
    BLAS-->>App: shared_ptr<BLAS>

    App->>TLAS: Create TLAS
    App->>TLAS: add_instance(blas, transform)
    App->>TLAS: add_instance(blas, transform)

    App->>CommandBuffer: begin()
    App->>TLAS: build(cmd)
    Note over TLAS: Record build commands
    App->>CommandBuffer: end()

    App->>GPU: Submit and wait
    GPU->>BLAS: Build BLAS
    GPU->>TLAS: Build TLAS
    GPU-->>App: Complete
```

## Template 6: Error Handling Sequence

Use for documenting error flows.

```mermaid
sequenceDiagram
    participant App
    participant Vulkan
    participant Exception

    App->>Vulkan: vkCreateDevice()
    alt Success
        Vulkan-->>App: VK_SUCCESS
    else Failure
        Vulkan-->>App: VK_ERROR_*
        App->>Exception: throw VulkanException
        Exception-->>App: Unwind stack
        Note over App: RAII cleanup
    end
```

## Template 7: Resource Tracking Sequence

Use for documenting automatic barrier generation.

```mermaid
sequenceDiagram
    participant App
    participant Tracker as ResourceTracker
    participant CommandBuffer

    App->>Tracker: track(ImageState: Undefined)
    Note over Tracker: Record current state

    App->>Tracker: request(ImageState: ColorAttachment)
    Note over Tracker: Queue state transition

    App->>Tracker: flush(cmd)
    Tracker->>Tracker: Compute minimal barriers
    Tracker->>CommandBuffer: Record vkCmdPipelineBarrier2

    Note over App: Image now in ColorAttachment layout
```

## Customization Guide

1. **Use participants** for components
2. **Use activate/deactivate** for lifetimes
3. **Use notes** for important details
4. **Use alt/else** for conditional paths
5. **Use loops** for repeated operations
6. **Keep messages concise** - one action per arrow
