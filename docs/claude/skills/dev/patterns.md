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

```cpp
auto instance = InstanceBuilder().setDebug().setApiVersion(ApiVersion::e13).build();
auto device = instance->findGpu().with_queue(eGraphics).with_synchronization_2().with_dynamic_rendering().build();
auto imageView = ImageViewBuilder(device, image).set_aspect(eColor).build();
auto sampler = SamplerBuilder(device).set_filter(eLinear).build();
```

## Error Handling

```cpp
check_vk(result, "context");   // Throws VulkanException
check_vma(result, "context");  // Throws VMAException
check_sdl(ptr_or_bool, "context");  // Throws SDLException (pointer and bool overloads)

// Exception hierarchy: vw::Exception → VulkanException, VMAException, SDLException, FileException,
//   AssimpException, ShaderCompilationException, ValidationLayerException, SwapchainException, LogicException
```

## RAII Handles

```cpp
ObjectWithHandle<T>        // Non-owning (borrowed)
ObjectWithUniqueHandle<T>  // Owning (auto-destroyed)
```
