cmake_minimum_required(VERSION 3.20)

find_package(GTest CONFIG REQUIRED)

enable_testing()

# Memory tests
add_executable(MemoryTests
    Memory/IntervalTests.cpp
    Memory/IntervalSetTests.cpp
)

target_link_libraries(MemoryTests
    PRIVATE
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(MemoryTests)
