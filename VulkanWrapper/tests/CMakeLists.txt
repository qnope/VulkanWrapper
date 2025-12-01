cmake_minimum_required(VERSION 3.20)

find_package(GTest CONFIG REQUIRED)

enable_testing()

add_library(TestUtils STATIC
    utils/create_gpu.cpp
)

target_link_libraries(TestUtils
    PUBLIC
    VulkanWrapperCoreLibrary
)

target_include_directories(TestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Memory tests
add_executable(MemoryTests
    Memory/IntervalTests.cpp
    Memory/IntervalSetTests.cpp
    Memory/UniformBufferAllocatorTests.cpp
    Memory/ResourceTrackerTests.cpp
)

target_link_libraries(MemoryTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(MemoryTests)
