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
add_executable(MemoryTests
    Memory/IntervalTests.cpp
    Memory/IntervalSetTests.cpp
    Memory/UniformBufferAllocatorTests.cpp
    Memory/ResourceTrackerTests.cpp
    Memory/BufferTests.cpp
    Memory/AllocatorTests.cpp
    Memory/StagingBufferManagerTests.cpp
    Memory/TransferTests.cpp
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

# RayTracing tests
add_executable(RayTracingTests
    RayTracing/RayTracedSceneTests.cpp
)

target_link_libraries(RayTracingTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(RayTracingTests)

# Utils tests
add_executable(UtilsTests
    utils/ErrorTests.cpp
)

target_link_libraries(UtilsTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(UtilsTests)

# Command tests
add_executable(CommandTests
    Command/CommandPoolTests.cpp
)

target_link_libraries(CommandTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(CommandTests)

# Descriptor tests
add_executable(DescriptorTests
    Descriptors/DescriptorTests.cpp
)

target_link_libraries(DescriptorTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(DescriptorTests)

# Pipeline tests
add_executable(PipelineTests
    Pipeline/PipelineTests.cpp
)

target_link_libraries(PipelineTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(PipelineTests)

# Synchronization tests
add_executable(SynchronizationTests
    Synchronization/SynchronizationTests.cpp
)

target_link_libraries(SynchronizationTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(SynchronizationTests)
