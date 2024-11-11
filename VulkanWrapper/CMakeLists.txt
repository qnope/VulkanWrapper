cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  SDL
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG        release-2.30.3 # release-1.10.0
)

FetchContent_Declare(
    Assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG v5.4.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(SDL Assimp glm)

find_package(Vulkan REQUIRED)

add_library(VulkanWrapper SHARED "")

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapper PUBLIC include)
target_compile_features(VulkanWrapper PUBLIC cxx_std_23)

target_link_libraries(VulkanWrapper PUBLIC SDL2::SDL2 Vulkan::Vulkan assimp glm)

target_compile_options(VulkanWrapper PUBLIC -fsanitize=address)
target_link_options(VulkanWrapper PUBLIC -fsanitize=address)
target_precompile_headers(VulkanWrapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)
