cmake_minimum_required(VERSION 3.25)

include(FetchContent)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SDL3_image REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED)

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)

target_link_libraries(VulkanWrapperCoreLibrary PUBLIC Vulkan::Vulkan assimp::assimp glm::glm GPUOpen::VulkanMemoryAllocator)
target_link_libraries(VulkanWrapperCoreLibrary PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)
target_precompile_headers(VulkanWrapperCoreLibrary PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)

add_custom_command(TARGET VulkanWrapper::VW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:VulkanWrapper::VW> ${CMAKE_SOURCE_DIR}/vw_rust/)
