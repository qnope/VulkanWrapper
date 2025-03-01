#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

#include <algorithm>
#include <assimp/Importer.hpp>
#include <concepts>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <ranges>
#include <set>
#include <variant>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

template <typename... Fs> struct overloaded : Fs... {
    using Fs::operator()...;
};
