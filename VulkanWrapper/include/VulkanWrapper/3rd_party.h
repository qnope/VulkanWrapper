#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

#define GLM_FORCE_RADIANS
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
#include <vector>
#include <vulkan/vulkan.hpp>

template <typename... Fs> struct overloaded : Fs... {
    using Fs::operator()...;
};
