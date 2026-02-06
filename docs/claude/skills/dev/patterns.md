# Patterns

## ScreenSpacePass

Base class for fullscreen post-processing passes. Inherits lazy image allocation from `Subpass`.

```cpp
enum class Slots { Output };

class MyPass : public ScreenSpacePass<Slots> {
public:
    MyPass(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator)
        : ScreenSpacePass<Slots>(std::move(device), std::move(allocator)) {
        // Create pipeline, descriptor pool, etc. in constructor
    }

    void execute(vk::CommandBuffer cmd, vk::Extent2D extent, uint32_t frameIndex) {
        auto& output = get_or_create_image(Slots::Output, Width{extent.width}, Height{extent.height},
                                            frameIndex, format, usage);

        vk::RenderingAttachmentInfo colorAttachment{
            .imageView = output.view->handle(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore
        };

        render_fullscreen(cmd, extent, colorAttachment, nullptr,
                          *m_pipeline, m_descriptorSet, &pushConstants, sizeof(pushConstants));
    }
};
```

**Pipeline helper** (`ScreenSpacePass.h`):
```cpp
auto pipeline = create_screen_space_pipeline(
    device, vertexShader, fragmentShader, descriptorSetLayout,
    colorFormat,                              // color attachment format
    vk::Format::eUndefined,                   // depth format (eUndefined = no depth)
    vk::CompareOp::eAlways,                   // depth compare
    pushConstantRanges,                       // vector<vk::PushConstantRange>
    std::optional<ColorBlendConfig>{}         // blending (nullopt = no blend)
);
```

## MeshRenderer

Binds pipelines per `MaterialTypeTag` for bindless mesh rendering:
```cpp
vw::MeshRenderer renderer;
renderer.add_pipeline(material_tag, pipeline);
renderer.draw_mesh(cmd, mesh, transform);
auto pipeline = renderer.pipeline_for(tag);
```

## Builders

See CLAUDE.md for core builder examples. Additional builders:

```cpp
auto imageView = ImageViewBuilder(device, image).set_aspect(eColor).build();
auto sampler = SamplerBuilder(device).set_filter(eLinear).build();
auto cmdPool = CommandPoolBuilder(device).build();
auto fence = FenceBuilder(device).build();
```

## RAII Handles

`ObjectWithHandle<T>` is the base wrapper for all Vulkan handles. Ownership depends on the template parameter:

```cpp
ObjectWithHandle<vk::Pipeline>          // Non-owning (raw handle)
ObjectWithHandle<vk::UniquePipeline>    // Owning (auto-destroyed)
```

`ObjectWithUniqueHandle<T>` is a type alias for `ObjectWithHandle<T>` (not a separate class):
```cpp
template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;
```

The `to_handle` range adaptor extracts handles (defined in `Utils/Algos.h`):
```cpp
auto handles = objects | vw::to_handle | to<std::vector>;
```

## DescriptorSet with ResourceState

`DescriptorSet` carries resource states for automatic barrier tracking:
```cpp
class DescriptorSet : public ObjectWithHandle<vk::DescriptorSet> {
    const std::vector<Barrier::ResourceState>& resources() const;
};
// Use resources() to feed ResourceTracker when binding descriptors
```

## CachedImage

Returned by `Subpass::get_or_create_image()`:
```cpp
struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};
```
Images are cached by (slot, width, height, frame_index). Old images with different dimensions are auto-cleaned on resize.
