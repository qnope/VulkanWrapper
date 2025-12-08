cmake_minimum_required(VERSION 3.25)

project(VulkanWrapper)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True)

# Code coverage option (requires Clang)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

# Coverage interface target (always defined, usage is conditional)
add_library(coverage_options INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(coverage_options INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
    )
    target_link_options(coverage_options INTERFACE
        -fprofile-instr-generate
    )
elseif(ENABLE_COVERAGE)
    message(WARNING "Code coverage is only supported with Clang compiler")
endif()

enable_testing()
add_subdirectory(VulkanWrapper)
add_subdirectory(examples)



