cmake_minimum_required(VERSION 3.14)

add_library(VulkanWrapper_c SHARED
    include/array.h
    src/array.cpp)

add_subdirectory(src)
add_subdirectory(include)

target_include_directories(VulkanWrapper_c PUBLIC include)
target_link_libraries(VulkanWrapper_c PRIVATE VulkanWrapper)
target_precompile_headers(VulkanWrapper_c REUSE_FROM VulkanWrapper)

install(TARGETS VulkanWrapper_c DESTINATION .)
