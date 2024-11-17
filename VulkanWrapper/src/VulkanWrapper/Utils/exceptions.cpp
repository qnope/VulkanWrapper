#include <utility>
#include <VulkanWrapper/Utils/exceptions.h>

namespace vw {
Exception::Exception(std::source_location sourceLocation)
    : m_sourceLocation{std::move(sourceLocation)} {}
} // namespace vw
