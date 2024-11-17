#pragma once

#include <source_location>

namespace vw {
struct Exception {
    Exception(std::source_location location);

    std::source_location m_sourceLocation;
};

template <typename Tag> struct TaggedException : Exception {
    using Exception::Exception;
};

} // namespace vw
