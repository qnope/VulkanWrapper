#pragma once

namespace vw {
template <typename UniqueHandle> class ObjectWithUniqueHandle {
  public:
    ObjectWithUniqueHandle(UniqueHandle handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept { return *m_handle; }

  private:
    UniqueHandle m_handle;
};

template <typename Handle> class ObjectWithHandle {
  public:
    ObjectWithHandle(Handle handle)
        : m_handle{handle} {}

    auto handle() const noexcept { return m_handle; }

  private:
    Handle m_handle;
};

} // namespace vw
