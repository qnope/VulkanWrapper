cmake_minimum_required(VERSION 3.25)

include(FetchContent)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED)

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)
target_compile_definitions(VulkanWrapperCoreLibrary PRIVATE VW_LIB)
target_link_libraries(VulkanWrapperCoreLibrary
        PUBLIC
        glm::glm
        assimp::assimp
        SDL3::SDL3
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        SDL3_image::SDL3_image
        PRIVATE
        $<$<BOOL:${ENABLE_COVERAGE}>:coverage_options>
        unofficial::shaderc::shaderc
)

# Add tests subdirectory
add_subdirectory(tests)
target_link_libraries(VulkanWrapperCoreLibrary PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)
target_precompile_headers(VulkanWrapperCoreLibrary PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)


