add_library(VulkanWrapperWrapperLibrary SHARED)

add_subdirectory(src)
add_subdirectory(include)

target_include_directories(VulkanWrapperWrapperLibrary PUBLIC include)
target_link_libraries(VulkanWrapperWrapperLibrary PUBLIC VulkanWrapper::VW)
target_precompile_headers(VulkanWrapperWrapperLibrary REUSE_FROM VulkanWrapper::VW)

add_library(VulkanWrapper::Wrapper ALIAS VulkanWrapperWrapperLibrary)

add_custom_command(TARGET VulkanWrapper::Wrapper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:VulkanWrapper::Wrapper> ${CMAKE_SOURCE_DIR}/vw_rust/)
