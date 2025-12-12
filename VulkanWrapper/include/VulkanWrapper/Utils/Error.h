#pragma once

#include <exception>
#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vw {

// Base exception - all vw exceptions derive from this
class Exception : public std::exception {
public:
    explicit Exception(std::string message,
                       std::source_location location = std::source_location::current());

    const char *what() const noexcept override;
    const std::source_location &location() const noexcept;
    const std::string &message() const noexcept;

protected:
    virtual void build_what() const;

    std::string m_message;
    std::source_location m_location;
    mutable std::string m_what_cache;
};

// Vulkan API errors (VkResult != eSuccess)
// Captures full error chain when available via Vulkan-Hpp error handling
class VulkanException : public Exception {
public:
    VulkanException(vk::Result result,
                    std::string context,
                    std::source_location location = std::source_location::current());

    vk::Result result() const noexcept;
    const std::string &result_string() const noexcept;

protected:
    void build_what() const override;

private:
    vk::Result m_result;
    std::string m_result_string;
};

// SDL errors (automatically captures SDL_GetError())
class SDLException : public Exception {
public:
    explicit SDLException(std::string context,
                          std::source_location location = std::source_location::current());

    const std::string &sdl_error() const noexcept;

protected:
    void build_what() const override;

private:
    std::string m_sdl_error;
};

// VMA allocation errors
class VMAException : public Exception {
public:
    VMAException(VkResult result,
                 std::string context,
                 std::source_location location = std::source_location::current());

    VkResult result() const noexcept;
    const std::string &result_string() const noexcept;

protected:
    void build_what() const override;

private:
    VkResult m_result;
    std::string m_result_string;
};

// File I/O errors (file not found, invalid format, etc.)
class FileException : public Exception {
public:
    FileException(std::filesystem::path path,
                  std::string context,
                  std::source_location location = std::source_location::current());

    const std::filesystem::path &path() const noexcept;

protected:
    void build_what() const override;

private:
    std::filesystem::path m_path;
};

// Assimp model loading errors
class AssimpException : public Exception {
public:
    AssimpException(std::string assimp_error,
                    std::filesystem::path path,
                    std::source_location location = std::source_location::current());

    const std::string &assimp_error() const noexcept;
    const std::filesystem::path &path() const noexcept;

protected:
    void build_what() const override;

private:
    std::string m_assimp_error;
    std::filesystem::path m_path;
};

// Shader compilation errors (GLSL to SPIR-V)
class ShaderCompilationException : public Exception {
public:
    ShaderCompilationException(std::string shader_name,
                               vk::ShaderStageFlagBits stage,
                               std::string compilation_log,
                               std::source_location location = std::source_location::current());

    const std::string &shader_name() const noexcept;
    vk::ShaderStageFlagBits stage() const noexcept;
    const std::string &compilation_log() const noexcept;

protected:
    void build_what() const override;

private:
    std::string m_shader_name;
    vk::ShaderStageFlagBits m_stage;
    std::string m_compilation_log;
};

// Logic/state errors (invalid arguments, precondition violations, bounds errors)
// Replaces std::out_of_range and std::runtime_error for consistency
class LogicException : public Exception {
public:
    using Exception::Exception;

    // Factory methods for common cases
    static LogicException out_of_range(
        std::string_view what_is_invalid,
        std::size_t value,
        std::size_t max,
        std::source_location location = std::source_location::current());

    static LogicException invalid_state(
        std::string_view context,
        std::source_location location = std::source_location::current());

    static LogicException null_pointer(
        std::string_view what_is_null,
        std::source_location location = std::source_location::current());
};

// Helper functions (no macros) - use std::source_location default argument

// Vulkan result checking
inline void check_vk(vk::Result result,
                     std::string_view context,
                     std::source_location loc = std::source_location::current()) {
    if (result != vk::Result::eSuccess) {
        throw VulkanException(result, std::string(context), loc);
    }
}

// Overload for structured bindings: auto [result, value] = ...
template <typename T>
T check_vk(std::pair<vk::Result, T> result_pair,
           std::string_view context,
           std::source_location loc = std::source_location::current()) {
    if (result_pair.first != vk::Result::eSuccess) {
        throw VulkanException(result_pair.first, std::string(context), loc);
    }
    return std::move(result_pair.second);
}

// Overload for vk::ResultValue (Vulkan-Hpp's result type)
template <typename T>
T check_vk(vk::ResultValue<T> result_value,
           std::string_view context,
           std::source_location loc = std::source_location::current()) {
    if (result_value.result != vk::Result::eSuccess) {
        throw VulkanException(result_value.result, std::string(context), loc);
    }
    return std::move(result_value.value);
}

// SDL boolean result checking
inline void check_sdl(bool success,
                      std::string_view context,
                      std::source_location loc = std::source_location::current()) {
    if (!success) {
        throw SDLException(std::string(context), loc);
    }
}

// SDL pointer result checking (returns non-null pointer or throws)
template <typename T>
T *check_sdl(T *ptr,
             std::string_view context,
             std::source_location loc = std::source_location::current()) {
    if (ptr == nullptr) {
        throw SDLException(std::string(context), loc);
    }
    return ptr;
}

// VMA result checking
inline void check_vma(VkResult result,
                      std::string_view context,
                      std::source_location loc = std::source_location::current()) {
    if (result != VK_SUCCESS) {
        throw VMAException(result, std::string(context), loc);
    }
}

} // namespace vw
