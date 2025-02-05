cmake_minimum_required(VERSION 3.5)

project(VulkanWrapper)

add_subdirectory(VulkanWrapper)
add_subdirectory(vw_c)

add_executable(Main main.cpp)
target_link_libraries(Main PRIVATE VulkanWrapper)
target_precompile_headers(Main REUSE_FROM VulkanWrapper)
