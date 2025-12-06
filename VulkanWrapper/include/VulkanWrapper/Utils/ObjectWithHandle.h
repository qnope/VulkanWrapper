#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw {
template <typename T> class ObjectWithHandle {
  public:
    ObjectWithHandle(auto handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept {
        if constexpr (sizeof(T) > sizeof(void *))
            return *m_handle;
        else
            return m_handle;
    }

  private:
    T m_handle;
};

template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

constexpr auto to_handle = std::views::transform([](const auto &x) {
    if constexpr (requires { x.handle(); }) {
        return x.handle();
    } else {
        return x->handle();
    }
});

} // namespace vw
