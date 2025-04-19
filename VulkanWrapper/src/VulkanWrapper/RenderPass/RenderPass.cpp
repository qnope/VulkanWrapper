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

void RenderPass::execute(vk::CommandBuffer cmd_buffer,
                         const Framebuffer &framebuffer) {
    const auto renderPassBeginInfo =
        vk::RenderPassBeginInfo()
            .setRenderPass(handle())
            .setFramebuffer(framebuffer.handle())
            .setRenderArea(vk::Rect2D(vk::Offset2D(), framebuffer.extent2D()))
            .setClearValues(m_clear_values);

    const auto subpassInfo =
        vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

    cmd_buffer.beginRenderPass2(renderPassBeginInfo, subpassInfo);
    for (auto &subpass :
         m_subpasses | std::views::take(m_subpasses.size() - 1)) {
        subpass->execute(cmd_buffer, framebuffer);
        cmd_buffer.nextSubpass2(subpassInfo, vk::SubpassEndInfo());
    }
    m_subpasses.back()->execute(cmd_buffer, framebuffer);
    cmd_buffer.endRenderPass2(vk::SubpassEndInfo());
}

vk::SubpassDescription2
subpassToDescription(const std::unique_ptr<Subpass> &subpass) {
    const auto result =
        vk::SubpassDescription2()
            .setPipelineBindPoint(subpass->pipeline_bind_point())
            .setColorAttachments(subpass->color_attachments())
            .setInputAttachments(subpass->input_attachments())
            .setPDepthStencilAttachment(subpass->depth_stencil_attachment());

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
RenderPassBuilder::add_subpass(SubpassTag tag,
                               std::unique_ptr<Subpass> subpass) && {
    m_subpasses.emplace_back(tag, std::move(subpass));
    return std::move(*this);
}

RenderPass RenderPassBuilder::build() && {
    using namespace std::views;
    auto subpasses = m_subpasses | values | as_rvalue | to<std::vector>;
    const auto vkSubpassDescriptions =
        subpasses | transform(subpassToDescription) | to<std::vector>;

    const auto info = vk::RenderPassCreateInfo2()
                          .setAttachments(m_attachments)
                          .setSubpasses(vkSubpassDescriptions)
                          .setDependencies(m_subpass_dependencies);

    auto [result, renderPass] =
        m_device->handle().createRenderPass2Unique(info);

    if (result != vk::Result::eSuccess) {
        throw RenderPassCreationException{std::source_location::current()};
    }
    return RenderPass{std::move(renderPass), std::move(m_attachments),
                      std::move(m_clear_values), std::move(subpasses)};
}

RenderPassBuilder &&RenderPassBuilder::add_dependency(SubpassTag src,
                                                      SubpassTag dst) && {
    using namespace std::views;
    auto index_src = index_of(m_subpasses | std::views::keys, src);
    auto index_dst = index_of(m_subpasses | std::views::keys, dst);

    assert(index_src);
    assert(index_dst);

    vk::SubpassDependency2 dependency{};
    auto [src_stage, src_access] =
        m_subpasses[*index_src].second->output_dependencies();
    auto [dst_stage, dst_access] =
        m_subpasses[*index_dst].second->input_dependencies();

    dependency.setSrcSubpass(*index_src)
        .setDstSubpass(*index_dst)
        .setSrcStageMask(src_stage)
        .setSrcAccessMask(src_access)
        .setDstStageMask(dst_stage)
        .setDstAccessMask(dst_access);
    m_subpass_dependencies.push_back(dependency);
    return std::move(*this);
}

} // namespace vw
