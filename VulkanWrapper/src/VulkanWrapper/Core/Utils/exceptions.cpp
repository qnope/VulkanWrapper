#include <VulkanWrapper/Core/Utils/exceptions.h>
#include <utility>

namespace vw {
Exception::Exception(std::source_location sourceLocation)
    : m_sourceLocation{std::move(sourceLocation)} {}
} // namespace vw
