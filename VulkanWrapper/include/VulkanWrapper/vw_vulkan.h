#pragma once

// This header is compiled as a C++20 header unit (FILE_SET CXX_MODULE_HEADER_UNITS).
// It is imported by all module interface and implementation units via:
//   import "VulkanWrapper/vw_vulkan.h";
//
// Purpose: centralise all heavy third-party includes so that they are parsed
// only ONCE instead of once per translation unit.

// ---- Vulkan-Hpp configuration macros ----
// These must be defined before including vulkan.hpp.
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

// ---- GLM configuration macros ----
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// ---- Standard library ----
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

// ---- Vulkan-Hpp (the heavy include) ----
#include <vulkan/vulkan.hpp>

// ---- Vulkan Memory Allocator (declarations only, no VMA_IMPLEMENTATION) ----
#include <vk_mem_alloc.h>

// ---- GLM ----
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
