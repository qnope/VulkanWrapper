#pragma once

#include "Application.h"
#include <VulkanWrapper/Memory/Transfer.h>
#include <memory>

namespace vw {
class ImageView;
}

class ExampleRunner {
  public:
    explicit ExampleRunner(App &app);
    virtual ~ExampleRunner() = default;

    void run();

  protected:
    virtual void setup() = 0;
    virtual void render(vk::CommandBuffer cmd,
                        vw::Transfer &transfer,
                        uint32_t frame_index) = 0;

    virtual std::shared_ptr<const vw::ImageView> result_image();
    virtual void on_resize(vw::Width width, vw::Height height);
    virtual bool should_screenshot() const;

    App &app();
    vw::Transfer &transfer();
    int frame_count() const;

  private:
    App &m_app;
    vw::Transfer m_transfer;
    int m_frame_count = 0;
};
