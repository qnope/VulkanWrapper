#include "VulkanWrapper/RenderPass/RenderPass.h"

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

RenderPass::RenderPass(vk::UniqueRenderPass render_pass,
                       std::vector<vk::AttachmentDescription2> attachments,
                       std::vector<vk::ClearValue> clear_values,
                       std::vector<std::unique_ptr<Subpass>> subpasses)
    : ObjectWithUniqueHandle<vk::UniqueRenderPass>{std::move(render_pass)}
    , m_attachments{std::move(attachments)}
    , m_clear_values{std::move(clear_values)}
    , m_subpasses{std::move(subpasses)} {
    for (auto &subpass : m_subpasses)
        subpass->initialize(*this);
}

const std::vector<vk::ClearValue> &RenderPass::clear_values() const noexcept {
    return m_clear_values;
}

void RenderPass::execute(
    vk::CommandBuffer cmd_buffer, const Framebuffer &framebuffer,
    const std::span<const vk::DescriptorSet> first_descriptor_sets) {
    const auto renderPassBeginInfo =
        vk::RenderPassBeginInfo()
            .setRenderPass(handle())
            .setFramebuffer(framebuffer.handle())
            .setRenderArea(vk::Rect2D(vk::Offset2D(), framebuffer.extent2D()))
            .setClearValues(m_clear_values);

    const auto subpassInfo =
        vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

    cmd_buffer.beginRenderPass2(renderPassBeginInfo, subpassInfo);
    m_subpasses.front()->execute(cmd_buffer, first_descriptor_sets);
    cmd_buffer.endRenderPass2(vk::SubpassEndInfo());
}

struct SubpassDescription {
    std::vector<vk::AttachmentReference2> color_attachments;
    std::optional<vk::AttachmentReference2> depth_attachment;
};

vk::SubpassDescription2
subpassToDescription(const std::unique_ptr<Subpass> &subpass) {
    auto result = vk::SubpassDescription2()
                      .setPipelineBindPoint(subpass->pipeline_bind_point())
                      .setColorAttachments(subpass->color_attachments());

    if (const auto *depth = subpass->depth_stencil_attachment())
        result.setPDepthStencilAttachment(depth);

    return result;
}

RenderPassBuilder::RenderPassBuilder(const Device &device)
    : m_device{&device} {}

RenderPassBuilder &&
RenderPassBuilder::add_attachment(vk::AttachmentDescription2 attachment,
                                  vk::ClearValue clear_value) && {
    m_attachments.push_back(attachment);
    m_clear_values.push_back(clear_value);
    return std::move(*this);
}

RenderPassBuilder &&
RenderPassBuilder::add_subpass(std::unique_ptr<Subpass> subpass) && {
    m_subpasses.push_back(std::move(subpass));
    return std::move(*this);
}

RenderPass RenderPassBuilder::build() && {
    const auto vkSubpassDescriptions =
        m_subpasses | std::views::transform(subpassToDescription) |
        to<std::vector>;

    const auto info = vk::RenderPassCreateInfo2()
                          .setAttachments(m_attachments)
                          .setSubpasses(vkSubpassDescriptions);

    auto [result, renderPass] =
        m_device->handle().createRenderPass2Unique(info);

    if (result != vk::Result::eSuccess) {
        throw RenderPassCreationException{std::source_location::current()};
    }
    return RenderPass{std::move(renderPass), std::move(m_attachments),
                      std::move(m_clear_values), std::move(m_subpasses)};
}

} // namespace vw
