#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using RenderPassCreationException =
    TaggedException<struct RenderPassCreationTag>;

class IRenderPass : public ObjectWithUniqueHandle<vk::UniqueRenderPass> {
  public:
    IRenderPass(vk::UniqueRenderPass render_pass,
                std::vector<vk::ClearValue> clear_values);

    [[nodiscard]] const std::vector<vk::ClearValue> &
    clear_values() const noexcept;

  private:
    std::vector<vk::ClearValue> m_clear_values;
};

template <typename RenderPassInformation>
class RenderPass : public IRenderPass {
    friend class RenderPassBuilder;

  public:
    RenderPass(
        vk::UniqueRenderPass render_pass,
        std::vector<vk::AttachmentDescription2> attachments,
        std::vector<vk::ClearValue> clear_values,
        std::vector<std::unique_ptr<Subpass<RenderPassInformation>>> subpasses)
        : IRenderPass{std::move(render_pass), std::move(clear_values)}
        , m_attachments{std::move(attachments)}
        , m_subpasses{std::move(subpasses)} {
        for (auto &subpass : m_subpasses)
            subpass->initialize(*this);
    }

    void execute(vk::CommandBuffer cmd_buffer,
                 const vw::Framebuffer &framebuffer,
                 const RenderPassInformation &render_pass_info) {
        const auto renderPassBeginInfo =
            vk::RenderPassBeginInfo()
                .setRenderPass(handle())
                .setFramebuffer(framebuffer.handle())
                .setRenderArea(
                    vk::Rect2D(vk::Offset2D(), framebuffer.extent2D()))
                .setClearValues(clear_values());

        const auto subpassInfo =
            vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

        cmd_buffer.beginRenderPass2(renderPassBeginInfo, subpassInfo);
        for (auto &subpass :
             m_subpasses | std::views::take(m_subpasses.size() - 1)) {
            subpass->execute(cmd_buffer, render_pass_info);
            cmd_buffer.nextSubpass2(subpassInfo, vk::SubpassEndInfo());
        }
        m_subpasses.back()->execute(cmd_buffer, render_pass_info);
        cmd_buffer.endRenderPass2(vk::SubpassEndInfo());
    }

  private:
    std::vector<vk::AttachmentDescription2> m_attachments;
    std::vector<std::unique_ptr<Subpass<RenderPassInformation>>> m_subpasses;
};

class RenderPassBuilder {
  public:
    RenderPassBuilder(const Device &device);
    [[nodiscard]] RenderPassBuilder &&
    add_attachment(vk::AttachmentDescription2 attachment,
                   vk::ClearValue clear_value) &&;

    [[nodiscard]] RenderPassBuilder &&
    add_subpass(SubpassTag tag, std::unique_ptr<ISubpass> subpass) &&;

    [[nodiscard]] RenderPassBuilder &&add_dependency(SubpassTag src,
                                                     SubpassTag dst) &&;

    template <typename RenderPassInformation>
    [[nodiscard]] RenderPass<RenderPassInformation> build() && {
        auto render_pass = build_underlying();
        std::vector<std::unique_ptr<Subpass<RenderPassInformation>>> subpasses;
        for (auto &[tag, subpass] : m_subpasses) {
            auto concrete = dynamic_cast<Subpass<RenderPassInformation> *>(
                subpass.release());
            assert(concrete != nullptr);
            subpasses.emplace_back(concrete);
        }

        return RenderPass<RenderPassInformation>{
            std::move(render_pass), std::move(m_attachments),
            std::move(m_clear_values), std::move(subpasses)};
    }

  private:
    vk::UniqueRenderPass build_underlying();

  private:
    const Device *m_device;
    std::vector<vk::AttachmentDescription2> m_attachments;
    std::vector<vk::ClearValue> m_clear_values;
    std::vector<std::pair<SubpassTag, std::unique_ptr<ISubpass>>> m_subpasses;
    std::vector<vk::SubpassDependency2> m_subpass_dependencies;
};
} // namespace vw
