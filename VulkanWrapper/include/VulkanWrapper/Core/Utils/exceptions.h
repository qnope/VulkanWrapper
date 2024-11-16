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

using InitializationException = TaggedException<struct InitializationTag>;
using WindowInitializationException =
    TaggedException<struct WindowInitializationTag>;

using InstanceCreationException = TaggedException<struct InstanceCreationTag>;
using DeviceCreationException = TaggedException<struct DeviceCreationTag>;
using DeviceNotFoundException = TaggedException<struct DeviceNotFoundTag>;
using InvalidEnumException = TaggedException<struct InvalidEnumTag>;
using SurfaceCreationException = TaggedException<struct SurfaceCreationTag>;
using ImageViewCreationException = TaggedException<struct ImageViewCreationTag>;

} // namespace vw
