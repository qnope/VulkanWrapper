#pragma once

namespace r3d {
template <typename UniqueHandle> class ObjectWithUniqueHandle {
  public:
    ObjectWithUniqueHandle(UniqueHandle handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const { return *m_handle; }

  private:
    UniqueHandle m_handle;
};

} // namespace r3d
