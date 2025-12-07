#include "VulkanWrapper/Utils/Error.h"

#include <SDL3/SDL_error.h>
#include <format>
#include <vulkan/vulkan_to_string.hpp>

namespace vw {

namespace {

std::string_view extract_filename(const char *path) {
    std::string_view full_path(path);
    auto pos = full_path.find_last_of("/\\");
    if (pos != std::string_view::npos) {
        return full_path.substr(pos + 1);
    }
    return full_path;
}

std::string vk_result_to_string(VkResult result) {
    return vk::to_string(static_cast<vk::Result>(result));
}

} // namespace

// Exception base class

Exception::Exception(std::string message, std::source_location location)
    : m_message(std::move(message)), m_location(location) {}

const char *Exception::what() const noexcept {
    if (m_what_cache.empty()) {
        build_what();
    }
    return m_what_cache.c_str();
}

const std::source_location &Exception::location() const noexcept {
    return m_location;
}

const std::string &Exception::message() const noexcept {
    return m_message;
}

void Exception::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message);
}

// VulkanException

VulkanException::VulkanException(vk::Result result,
                                 std::string context,
                                 std::source_location location)
    : Exception(std::move(context), location),
      m_result(result),
      m_result_string(vk::to_string(result)) {}

vk::Result VulkanException::result() const noexcept {
    return m_result;
}

const std::string &VulkanException::result_string() const noexcept {
    return m_result_string;
}

void VulkanException::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message,
                               m_result_string);
}

// SDLException

SDLException::SDLException(std::string context, std::source_location location)
    : Exception(std::move(context), location), m_sdl_error(SDL_GetError()) {}

const std::string &SDLException::sdl_error() const noexcept {
    return m_sdl_error;
}

void SDLException::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message,
                               m_sdl_error);
}

// VMAException

VMAException::VMAException(VkResult result,
                           std::string context,
                           std::source_location location)
    : Exception(std::move(context), location),
      m_result(result),
      m_result_string(vk_result_to_string(result)) {}

VkResult VMAException::result() const noexcept {
    return m_result;
}

const std::string &VMAException::result_string() const noexcept {
    return m_result_string;
}

void VMAException::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message,
                               m_result_string);
}

// FileException

FileException::FileException(std::filesystem::path path,
                             std::string context,
                             std::source_location location)
    : Exception(std::move(context), location), m_path(std::move(path)) {}

const std::filesystem::path &FileException::path() const noexcept {
    return m_path;
}

void FileException::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message,
                               m_path.string());
}

// AssimpException

AssimpException::AssimpException(std::string assimp_error,
                                 std::filesystem::path path,
                                 std::source_location location)
    : Exception("Failed to load model", location),
      m_assimp_error(std::move(assimp_error)),
      m_path(std::move(path)) {}

const std::string &AssimpException::assimp_error() const noexcept {
    return m_assimp_error;
}

const std::filesystem::path &AssimpException::path() const noexcept {
    return m_path;
}

void AssimpException::build_what() const {
    m_what_cache = std::format("[{}:{}] {}\n  {}\n  {}\n  {}",
                               extract_filename(m_location.file_name()),
                               m_location.line(),
                               m_location.function_name(),
                               m_message,
                               m_path.string(),
                               m_assimp_error);
}

// LogicException factory methods

LogicException LogicException::out_of_range(std::string_view what_is_invalid,
                                            std::size_t value,
                                            std::size_t max,
                                            std::source_location location) {
    return LogicException(
        std::format("{} {} is out of range [0, {})", what_is_invalid, value, max),
        location);
}

LogicException LogicException::invalid_state(std::string_view context,
                                             std::source_location location) {
    return LogicException(std::string(context), location);
}

LogicException LogicException::null_pointer(std::string_view what_is_null,
                                            std::source_location location) {
    return LogicException(std::format("{} is null", what_is_null), location);
}

} // namespace vw
