#pragma once

namespace vw {
class SDL_Initializer;
class Window;

class StagingBufferManager;

class Instance;
class PhysicalDevice;
class DeviceFinder;
class Device;
class Queue;
class PresentQueue;
class Image;
class ImageView;
class Sampler;
class CombinedImage;

class SwapchainBuilder;
class Swapchain;
class Surface;

class ShaderModule;
class Pipeline;
class PipelineLayout;

class Attachment;
class Subpass;
class RenderPass;

class Semaphore;
class Fence;

class Allocator;

enum class QueueType;

class DescriptorSetLayout;

namespace Model::Material {
class Material;
}
} // namespace vw
