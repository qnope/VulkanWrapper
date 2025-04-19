#pragma once

namespace vk {
class DispatchLoaderDynamic;
}

namespace vw {
vk::DispatchLoaderDynamic &DefaultDispatcher();
}

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

#ifndef VW_LIB
#define VULKAN_HPP_DEFAULT_DISPATCHER vw::DefaultDispatcher()
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <algorithm>
#include <concepts>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <ranges>
#include <set>
#include <variant>
#include <numeric>
#include <vector>
#include <vulkan/vulkan.hpp>

template <typename... Fs> struct overloaded : Fs... {
    using Fs::operator()...;
};

namespace vw {
enum ApiVersion {
    e10 = vk::ApiVersion10,
    e11 = vk::ApiVersion11,
    e12 = vk::ApiVersion12,
    e13 = vk::ApiVersion13
};

enum class Width {};
enum class Height {};
enum class Depth {};
enum class MipLevel {};
} // namespace vw
