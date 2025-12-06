cmake_minimum_required(VERSION 3.25)

project(VulkanWrapper)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True)

enable_testing()
add_subdirectory(VulkanWrapper)
add_subdirectory(examples)



