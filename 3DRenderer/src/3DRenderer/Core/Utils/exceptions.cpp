#include <3DRenderer/Core/Utils/exceptions.h>
#include <utility>

namespace r3d {
Exception::Exception(std::source_location sourceLocation)
    : m_sourceLocation{std::move(sourceLocation)} {}
} // namespace r3d
