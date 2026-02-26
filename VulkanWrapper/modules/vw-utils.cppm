module;
#include "VulkanWrapper/3rd_party.h"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

export module vw:utils;
import :core;

export namespace vw {

// ---- Error hierarchy ----

class Exception : public std::exception {
  public:
    explicit Exception(
        std::string message,
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

class VulkanException : public Exception {
  public:
    VulkanException(
        vk::Result result, std::string context,
        std::source_location location = std::source_location::current());

    vk::Result result() const noexcept;
    const std::string &result_string() const noexcept;

  protected:
    void build_what() const override;

  private:
    vk::Result m_result;
    std::string m_result_string;
};

class SDLException : public Exception {
  public:
    explicit SDLException(
        std::string context,
        std::source_location location = std::source_location::current());

    const std::string &sdl_error() const noexcept;

  protected:
    void build_what() const override;

  private:
    std::string m_sdl_error;
};

class VMAException : public Exception {
  public:
    VMAException(
        VkResult result, std::string context,
        std::source_location location = std::source_location::current());

    VkResult result() const noexcept;
    const std::string &result_string() const noexcept;

  protected:
    void build_what() const override;

  private:
    VkResult m_result;
    std::string m_result_string;
};

class FileException : public Exception {
  public:
    FileException(
        std::filesystem::path path, std::string context,
        std::source_location location = std::source_location::current());

    const std::filesystem::path &path() const noexcept;

  protected:
    void build_what() const override;

  private:
    std::filesystem::path m_path;
};

class AssimpException : public Exception {
  public:
    AssimpException(
        std::string assimp_error, std::filesystem::path path,
        std::source_location location = std::source_location::current());

    const std::string &assimp_error() const noexcept;
    const std::filesystem::path &path() const noexcept;

  protected:
    void build_what() const override;

  private:
    std::string m_assimp_error;
    std::filesystem::path m_path;
};

class ShaderCompilationException : public Exception {
  public:
    ShaderCompilationException(
        std::string shader_name, vk::ShaderStageFlagBits stage,
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

class ValidationLayerException : public Exception {
  public:
    ValidationLayerException(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type, std::string validation_message,
        std::source_location location = std::source_location::current());

    vk::DebugUtilsMessageSeverityFlagBitsEXT severity() const noexcept;
    vk::DebugUtilsMessageTypeFlagsEXT type() const noexcept;
    const std::string &validation_message() const noexcept;

  protected:
    void build_what() const override;

  private:
    vk::DebugUtilsMessageSeverityFlagBitsEXT m_severity;
    vk::DebugUtilsMessageTypeFlagsEXT m_type;
    std::string m_validation_message;
};

class SwapchainException : public Exception {
  public:
    SwapchainException(
        vk::Result result, std::string context,
        std::source_location location = std::source_location::current());

    vk::Result result() const noexcept;

    [[nodiscard]] bool needs_recreation() const noexcept {
        return m_result == vk::Result::eErrorOutOfDateKHR ||
               m_result == vk::Result::eSuboptimalKHR;
    }

    [[nodiscard]] bool is_out_of_date() const noexcept {
        return m_result == vk::Result::eErrorOutOfDateKHR;
    }

    [[nodiscard]] bool is_suboptimal() const noexcept {
        return m_result == vk::Result::eSuboptimalKHR;
    }

  protected:
    void build_what() const override;

  private:
    vk::Result m_result;
};

class LogicException : public Exception {
  public:
    using Exception::Exception;

    static LogicException out_of_range(
        std::string_view what_is_invalid, std::size_t value, std::size_t max,
        std::source_location location = std::source_location::current());

    static LogicException invalid_state(
        std::string_view context,
        std::source_location location = std::source_location::current());

    static LogicException null_pointer(
        std::string_view what_is_null,
        std::source_location location = std::source_location::current());
};

// ---- check_* helper functions ----

inline void
check_vk(vk::Result result, std::string_view context,
         std::source_location loc = std::source_location::current()) {
    if (result != vk::Result::eSuccess) {
        throw VulkanException(result, std::string(context), loc);
    }
}

template <typename T>
T check_vk(std::pair<vk::Result, T> result_pair, std::string_view context,
           std::source_location loc = std::source_location::current()) {
    if (result_pair.first != vk::Result::eSuccess) {
        throw VulkanException(result_pair.first, std::string(context), loc);
    }
    return std::move(result_pair.second);
}

template <typename T>
T check_vk(vk::ResultValue<T> result_value, std::string_view context,
           std::source_location loc = std::source_location::current()) {
    if (result_value.result != vk::Result::eSuccess) {
        throw VulkanException(result_value.result, std::string(context), loc);
    }
    return std::move(result_value.value);
}

inline void
check_sdl(bool success, std::string_view context,
          std::source_location loc = std::source_location::current()) {
    if (!success) {
        throw SDLException(std::string(context), loc);
    }
}

template <typename T>
T *check_sdl(T *ptr, std::string_view context,
             std::source_location loc = std::source_location::current()) {
    if (ptr == nullptr) {
        throw SDLException(std::string(context), loc);
    }
    return ptr;
}

inline void
check_vma(VkResult result, std::string_view context,
          std::source_location loc = std::source_location::current()) {
    if (result != VK_SUCCESS) {
        throw VMAException(result, std::string(context), loc);
    }
}

// ---- ObjectWithHandle ----

template <typename T> class ObjectWithHandle {
  public:
    ObjectWithHandle(auto handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept {
        if constexpr (sizeof(T) > sizeof(void *))
            return *m_handle;
        else
            return m_handle;
    }

  private:
    T m_handle;
};

template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

constexpr auto to_handle = std::views::transform([](const auto &x) {
    if constexpr (requires { x.handle(); }) {
        return x.handle();
    } else {
        return x->handle();
    }
});

// ---- Algos ----

std::optional<int> index_of(const auto &range, const auto &object) {
    auto it = std::find(std::begin(range), std::end(range), object);
    if (it == std::end(range)) {
        return std::nullopt;
    }
    return std::distance(std::begin(range), it);
}

std::optional<int> index_if(const auto &range, const auto &predicate) {
    auto it = std::find_if(std::begin(range), std::end(range), predicate);
    if (it == std::end(range)) {
        return std::nullopt;
    }
    return std::distance(std::begin(range), it);
}

template <template <typename... Ts> typename Container> struct to_t {};

template <template <typename... Ts> typename Container>
constexpr to_t<Container> to{};

template <typename Range, template <typename... Ts> typename Container>
auto operator|(Range &&range, to_t<Container> /*unused*/) {
    return Container(std::forward<Range>(range).begin(),
                     std::forward<Range>(range).end());
}

template <typename T> struct construct_t {};

template <typename T> constexpr construct_t<T> construct{};

template <typename Range, typename T>
auto operator|(Range &&range, construct_t<T> /*unused*/) {
    auto constructor = [](const auto &x) {
        if constexpr (requires { T{x}; })
            return T{x};
        else
            return std::apply(
                [](auto &&...xs) {
                    return T{std::forward<decltype(xs)>(xs)...};
                },
                x);
    };
    return std::forward<Range>(range) | std::views::transform(constructor);
}

// ---- Alignment ----

constexpr uint64_t align(uint64_t size, uint64_t al) {
    return (size + al - 1) & ~(al - 1);
}

constexpr vk::DeviceSize DefaultBufferAlignment = 4'096;

} // namespace vw
