cmake_minimum_required(VERSION 3.20)

find_package(GTest CONFIG REQUIRED)

add_library(TestUtils STATIC
    utils/create_gpu.cpp
)

target_link_libraries(TestUtils
    PUBLIC
    VulkanWrapperCoreLibrary
)

target_include_directories(TestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Memory tests
set(MEMORY_TEST_SOURCES
    Memory/IntervalTests.cpp
    Memory/IntervalSetTests.cpp
    Memory/UniformBufferAllocatorTests.cpp
    Memory/ResourceTrackerTests.cpp
    Memory/BufferTests.cpp
    Memory/AllocatorTests.cpp
    Memory/StagingBufferManagerTests.cpp
)

if(TARGET SDL3_image::SDL3_image)
    list(APPEND MEMORY_TEST_SOURCES Memory/TransferTests.cpp)
endif()

add_executable(MemoryTests ${MEMORY_TEST_SOURCES})

target_link_libraries(MemoryTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(MemoryTests)

# Image tests
add_executable(ImageTests
    Image/ImageTests.cpp
    Image/ImageViewTests.cpp
    Image/SamplerTests.cpp
)

target_link_libraries(ImageTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(ImageTests)

# Vulkan tests
add_executable(VulkanTests
    Vulkan/InstanceTests.cpp
)

target_link_libraries(VulkanTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(VulkanTests)
