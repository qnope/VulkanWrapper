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

# Shader tests
add_executable(ShaderTests
    Shader/ShaderCompilerTests.cpp
)

target_link_libraries(ShaderTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(ShaderTests)

# RenderPass tests
add_executable(RenderPassTests
    RenderPass/SubpassTests.cpp
    RenderPass/ScreenSpacePassTests.cpp
    RenderPass/ToneMappingPassTests.cpp
    RenderPass/SkyPassTests.cpp
    RenderPass/SunLightPassTests.cpp
    RenderPass/SkyLightPassTests.cpp
)

target_link_libraries(RenderPassTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(RenderPassTests)

# Material tests
add_executable(MaterialTests
    Material/MaterialTypeTagTests.cpp
    Material/ColoredMaterialHandlerTests.cpp
    Material/TexturedMaterialHandlerTests.cpp
    Material/BindlessTextureManagerTests.cpp
    Material/BindlessMaterialManagerTests.cpp
)

target_link_libraries(MaterialTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(MaterialTests)

# Random tests
add_executable(RandomTests
    Random/RandomSamplingTests.cpp
)

target_link_libraries(RandomTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(RandomTests)
