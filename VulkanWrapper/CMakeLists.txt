cmake_minimum_required(VERSION 3.25)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)

add_library(VulkanWrapper SHARED "")

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapper PUBLIC include)
target_compile_features(VulkanWrapper PUBLIC cxx_std_23)

target_link_libraries(VulkanWrapper PUBLIC SDL3::SDL3 Vulkan::Vulkan assimp::assimp glm::glm)

#target_compile_options(VulkanWrapper PUBLIC -fsanitize=address)
#target_link_options(VulkanWrapper PUBLIC -fsanitize=address)
target_precompile_headers(VulkanWrapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)

install(TARGETS VulkanWrapper DESTINATION .)
