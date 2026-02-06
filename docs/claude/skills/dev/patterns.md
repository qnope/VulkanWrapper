# Patterns

## ScreenSpacePass

```cpp
enum class Slots { Output };

class MyPass : public ScreenSpacePass<Slots> {
public:
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

The `to_handle` range adaptor extracts handles:
```cpp
auto handles = objects | vw::to_handle | to<std::vector>;
```
