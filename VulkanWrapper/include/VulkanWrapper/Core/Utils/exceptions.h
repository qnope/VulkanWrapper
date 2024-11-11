#pragma once

#include <source_location>

namespace r3d {
struct Exception {
    Exception(std::source_location location);

    std::source_location m_sourceLocation;
};

struct InitializationException : public Exception {
    using Exception::Exception;
};

struct WindowInitializationException : public Exception {
    using Exception::Exception;
};

struct InstanceCreationException : public Exception {
    using Exception::Exception;
};

struct DeviceCreationException : public Exception {
    using Exception::Exception;
};

struct DeviceNotFoundException : public Exception {
    using Exception::Exception;
};

struct InvalidEnumException : public Exception {
    using Exception::Exception;
};

struct SurfaceCreationException : public Exception {
    using Exception::Exception;
};

} // namespace r3d
