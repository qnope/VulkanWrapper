cmake_minimum_required(VERSION 3.25)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED "")

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)

target_link_libraries(VulkanWrapperCoreLibrary PUBLIC SDL3::SDL3 Vulkan::Vulkan assimp::assimp glm::glm)

target_precompile_headers(VulkanWrapperCoreLibrary PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)

install(TARGETS VulkanWrapperCoreLibrary DESTINATION .)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)
