add_library(VulkanWrapperWrapperLibrary SHARED)

add_subdirectory(src)
add_subdirectory(include)

target_include_directories(VulkanWrapperWrapperLibrary PUBLIC include)
target_link_libraries(VulkanWrapperWrapperLibrary PUBLIC VulkanWrapper::VW)
target_precompile_headers(VulkanWrapperWrapperLibrary REUSE_FROM VulkanWrapper::VW)

install(TARGETS VulkanWrapperWrapperLibrary DESTINATION .)

add_library(VulkanWrapper::Wrapper ALIAS VulkanWrapperWrapperLibrary)
