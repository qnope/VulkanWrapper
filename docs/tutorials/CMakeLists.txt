# Tutorial Examples CMake Configuration
#
# This file builds all tutorial examples for documentation.
# Each tutorial is a minimal, self-contained example that produces a screenshot.

cmake_minimum_required(VERSION 3.25)

# Tutorial Framework Library
add_library(TutorialFramework STATIC
    TutorialFramework/TutorialApp.cpp
    TutorialFramework/TutorialApp.h
)

target_include_directories(TutorialFramework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TutorialFramework PUBLIC
    VulkanWrapperCoreLibrary
)

target_precompile_headers(TutorialFramework REUSE_FROM VulkanWrapperCoreLibrary)

# Helper function to add a tutorial executable
function(add_tutorial NAME)
    set(TARGET_NAME "Tutorial_${NAME}")

    # Collect source files
    file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.cpp")
    file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.h")

    # Create executable
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

    target_link_libraries(${TARGET_NAME} PRIVATE
        TutorialFramework
        VulkanWrapperCoreLibrary
    )

    target_precompile_headers(${TARGET_NAME} REUSE_FROM VulkanWrapperCoreLibrary)

    # Set output directory
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
    )

    # Compile shaders if present
    file(GLOB VERTEX_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/Shaders/*.vert")
    file(GLOB FRAGMENT_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/Shaders/*.frag")

    foreach(SHADER ${VERTEX_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        VwCompileShader(${TARGET_NAME} ${SHADER} "${SHADER_NAME}.spv")
    endforeach()

    foreach(SHADER ${FRAGMENT_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        VwCompileShader(${TARGET_NAME} ${SHADER} "${SHADER_NAME}.spv")
    endforeach()
endfunction()

# Add tutorial subdirectories
add_tutorial(01-triangle)
add_tutorial(02-texture)

# Target to run all tutorials and generate screenshots
add_custom_target(generate_screenshots
    COMMENT "Generating screenshots for all tutorials..."
)

# Add each tutorial as a dependency
add_dependencies(generate_screenshots
    Tutorial_01-triangle
    Tutorial_02-texture
)
