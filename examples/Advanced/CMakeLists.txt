add_executable(Main main.cpp)
target_sources(Main PRIVATE FILE_SET CXX_MODULES FILES
    Advanced.cppm
    RenderPassInformation.cppm
    SceneSetup.cppm
    ZPass.cppm
    DirectLightPass.cppm
    AmbientOcclusionPass.cppm
    DeferredRenderingManager.cppm
)
target_link_libraries(Main PRIVATE App)
