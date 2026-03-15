add_library(ExamplesCommon INTERFACE)
target_include_directories(ExamplesCommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ExamplesCommon INTERFACE VulkanWrapper::VW)
