#include <utility>
#include <VulkanWrapper/Utils/exceptions.h>

namespace vw {
Exception::Exception(std::source_location sourceLocation)
    : m_sourceLocation{sourceLocation} {}
} // namespace vw
