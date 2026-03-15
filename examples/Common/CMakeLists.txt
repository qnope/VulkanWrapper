add_library(ExamplesCommon STATIC ExampleRunner.cpp ExampleRunner.h SceneSetup.h)
target_include_directories(ExamplesCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ExamplesCommon PUBLIC VulkanWrapper::VW App)
target_precompile_headers(ExamplesCommon REUSE_FROM VulkanWrapperCoreLibrary)
