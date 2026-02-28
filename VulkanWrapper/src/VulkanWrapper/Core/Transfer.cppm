export module vw.memory:transfer;
import std3rd;
import vulkan3rd;
import vw.utils;
import vw.sync;
import vw.vulkan;
import :allocator;

export namespace vw {

class Transfer {
  public:
    Transfer() = default;

    void
    blit(vk::CommandBuffer cmd, const std::shared_ptr<const Image> &src,
         const std::shared_ptr<const Image> &dst,
         std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt,
         std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt,
         vk::Filter filter = vk::Filter::eLinear);

    void copyBuffer(vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst,
                    vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
                    vk::DeviceSize size);

    void copyBufferToImage(
        vk::CommandBuffer cmd, vk::Buffer src,
        const std::shared_ptr<const Image> &dst, vk::DeviceSize srcOffset,
        std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt);

    void copyImageToBuffer(
        vk::CommandBuffer cmd, const std::shared_ptr<const Image> &src,
        vk::Buffer dst, vk::DeviceSize dstOffset,
        std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt);

    [[nodiscard]] Barrier::ResourceTracker &resourceTracker() {
        return m_resourceTracker;
    }

    void
    saveToFile(vk::CommandBuffer cmd, const Allocator &allocator, Queue &queue,
               const std::shared_ptr<const Image> &image,
               const std::filesystem::path &path,
               vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

  private:
    Barrier::ResourceTracker m_resourceTracker;

    vk::ImageSubresourceRange
    getFullSubresourceRange(const std::shared_ptr<const Image> &image) const;
};

} // namespace vw
