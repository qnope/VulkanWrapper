
add_executable(Main main.cpp
    RenderPassInformation.h
    SceneSetup.h
)

target_link_libraries(Main PRIVATE App)
target_precompile_headers(Main REUSE_FROM VulkanWrapperCoreLibrary)

