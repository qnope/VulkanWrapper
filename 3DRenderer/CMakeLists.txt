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

add_library(3DRenderer SHARED "")

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(3DRenderer PUBLIC include)
target_compile_features(3DRenderer PUBLIC cxx_std_23)

target_link_libraries(3DRenderer PUBLIC SDL2::SDL2 Vulkan::Vulkan assimp glm)

target_compile_options(3DRenderer PUBLIC -fsanitize=address)
target_link_options(3DRenderer PUBLIC -fsanitize=address)
target_precompile_headers(3DRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/3DRenderer/3rd_party.h)
