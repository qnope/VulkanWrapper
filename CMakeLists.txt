cmake_minimum_required(VERSION 3.5)

project(3D_Renderer)

add_subdirectory(3DRenderer)

add_executable(Main main.cpp)
target_link_libraries(Main PRIVATE 3DRenderer)
target_precompile_headers(Main REUSE_FROM 3DRenderer)
