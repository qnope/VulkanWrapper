#pragma once

namespace r3d {
template <typename UniqueHandle, typename... Other>
class ObjectWithUniqueHandle {
  protected:
    ObjectWithUniqueHandle(UniqueHandle handle) noexcept
        : m_handle{std::move(handle)} {}

  public:
    auto handle() const noexcept { return *m_handle; }

  private:
    UniqueHandle m_handle;
};

template <typename UniqueHandle, typename Handle>
class ObjectWithUniqueHandle<UniqueHandle, Handle> {
  protected:
    ObjectWithUniqueHandle(UniqueHandle handle) noexcept
        : m_handle{std::move(handle)} {}

    ObjectWithUniqueHandle(Handle handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept {
        auto getter =
            overloaded{[](const Handle &handle) { return handle; },
                       [](const UniqueHandle &handle) { return *handle; }};

        return std::visit(getter, m_handle);
    }

  private:
    std::variant<UniqueHandle, Handle> m_handle;
};

} // namespace r3d