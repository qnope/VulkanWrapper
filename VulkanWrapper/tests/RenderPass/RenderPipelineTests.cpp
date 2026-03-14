#include "utils/create_gpu.hpp"
#include "VulkanWrapper/RenderPass/RenderPipeline.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <gtest/gtest.h>

namespace {

class MockPass : public vw::RenderPass {
  public:
    MockPass(std::shared_ptr<vw::Device> device,
             std::shared_ptr<vw::Allocator> allocator,
             std::vector<vw::Slot> inputs,
             std::vector<vw::Slot> outputs)
        : RenderPass(std::move(device), std::move(allocator))
        , m_inputs(std::move(inputs))
        , m_outputs(std::move(outputs)) {}

    std::vector<vw::Slot> input_slots() const override {
        return m_inputs;
    }
    std::vector<vw::Slot> output_slots() const override {
        return m_outputs;
    }
    std::string_view name() const override {
        return "MockPass";
    }

    void execute(vk::CommandBuffer cmd,
                 vw::Barrier::ResourceTracker &tracker,
                 vw::Width width, vw::Height height,
                 size_t frame_index) override {
        m_executed = true;
        for (auto slot : m_outputs) {
            get_or_create_image(
                slot, width, height, frame_index,
                vk::Format::eR8G8B8A8Unorm,
                vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eSampled);
        }
    }

    bool was_executed() const { return m_executed; }

    // Expose get_input for testing wiring
    const vw::CachedImage &test_get_input(vw::Slot slot) const {
        return get_input(slot);
    }

  private:
    std::vector<vw::Slot> m_inputs;
    std::vector<vw::Slot> m_outputs;
    bool m_executed = false;
};

class MockPassWithReset : public MockPass {
  public:
    using MockPass::MockPass;

    void reset_accumulation() override {
        m_reset_called = true;
    }

    bool was_reset_called() const { return m_reset_called; }

  private:
    bool m_reset_called = false;
};

class RenderPipelineTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
    }

    auto make_pass(std::vector<vw::Slot> inputs,
                   std::vector<vw::Slot> outputs) {
        return std::make_unique<MockPass>(
            device, allocator, std::move(inputs),
            std::move(outputs));
    }

    auto make_pass_with_reset(
        std::vector<vw::Slot> inputs,
        std::vector<vw::Slot> outputs) {
        return std::make_unique<MockPassWithReset>(
            device, allocator, std::move(inputs),
            std::move(outputs));
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
};

} // namespace

TEST_F(RenderPipelineTest, Validate_EmptyPipeline_Valid) {
    vw::RenderPipeline pipeline;
    auto result = pipeline.validate();
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(RenderPipelineTest,
       Validate_SinglePassNoInputs_Valid) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({}, {vw::Slot::Albedo}));
    auto result = pipeline.validate();
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(RenderPipelineTest, Validate_MissingInput_Invalid) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({vw::Slot::Depth}, {}));
    auto result = pipeline.validate();
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("Depth"),
              std::string::npos);
}

TEST_F(RenderPipelineTest,
       Validate_InputSatisfiedByPredecessor_Valid) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({}, {vw::Slot::Depth}));
    pipeline.add(make_pass({vw::Slot::Depth}, {}));
    auto result = pipeline.validate();
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(RenderPipelineTest,
       Validate_InputFromLaterPass_Invalid) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({vw::Slot::Depth}, {}));
    pipeline.add(make_pass({}, {vw::Slot::Depth}));
    auto result = pipeline.validate();
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("Depth"),
              std::string::npos);
}

TEST_F(RenderPipelineTest,
       Validate_MultipleErrors_AllReported) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass(
        {vw::Slot::Depth, vw::Slot::Albedo}, {}));
    auto result = pipeline.validate();
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.errors.size(), 2u);
}

TEST_F(RenderPipelineTest, Execute_AllPassesCalled) {
    vw::RenderPipeline pipeline;
    auto &pass_a = pipeline.add(
        make_pass({}, {vw::Slot::Albedo}));
    auto &pass_b = pipeline.add(
        make_pass({vw::Slot::Albedo}, {vw::Slot::ToneMapped}));

    auto &mock_a = static_cast<MockPass &>(pass_a);
    auto &mock_b = static_cast<MockPass &>(pass_b);

    vw::Barrier::ResourceTracker tracker;
    pipeline.execute(vk::CommandBuffer{}, tracker,
                     vw::Width{256}, vw::Height{256}, 0);

    EXPECT_TRUE(mock_a.was_executed());
    EXPECT_TRUE(mock_b.was_executed());
}

TEST_F(RenderPipelineTest, Execute_WiresOutputsToInputs) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({}, {vw::Slot::Albedo}));
    auto &pass_b = pipeline.add(
        make_pass({vw::Slot::Albedo}, {}));

    auto &mock_b = static_cast<MockPass &>(pass_b);

    vw::Barrier::ResourceTracker tracker;
    pipeline.execute(vk::CommandBuffer{}, tracker,
                     vw::Width{256}, vw::Height{256}, 0);

    // pass_b should have received the albedo image as input
    const auto &input =
        mock_b.test_get_input(vw::Slot::Albedo);
    EXPECT_NE(input.image, nullptr);
    EXPECT_NE(input.view, nullptr);
}

TEST_F(RenderPipelineTest,
       ResetAccumulation_CallsAllPasses) {
    vw::RenderPipeline pipeline;
    auto &pass_a = pipeline.add(
        make_pass_with_reset({}, {vw::Slot::Albedo}));
    auto &pass_b = pipeline.add(make_pass_with_reset(
        {vw::Slot::Albedo}, {vw::Slot::ToneMapped}));

    auto &mock_a =
        static_cast<MockPassWithReset &>(pass_a);
    auto &mock_b =
        static_cast<MockPassWithReset &>(pass_b);

    pipeline.reset_accumulation();

    EXPECT_TRUE(mock_a.was_reset_called());
    EXPECT_TRUE(mock_b.was_reset_called());
}

TEST_F(RenderPipelineTest, Add_ReturnsTypedReference) {
    vw::RenderPipeline pipeline;
    auto &ref =
        pipeline.add(make_pass({}, {vw::Slot::Albedo}));

    // The returned reference should be of the concrete type
    static_assert(
        std::is_same_v<decltype(ref), MockPass &>);
    EXPECT_EQ(ref.name(), "MockPass");
}

TEST_F(RenderPipelineTest, PassAccess) {
    vw::RenderPipeline pipeline;
    pipeline.add(make_pass({}, {vw::Slot::Albedo}));
    pipeline.add(make_pass({}, {vw::Slot::Normal}));

    EXPECT_EQ(pipeline.pass_count(), 2u);
    EXPECT_EQ(pipeline.pass(0).name(), "MockPass");
    EXPECT_EQ(pipeline.pass(1).name(), "MockPass");
}
