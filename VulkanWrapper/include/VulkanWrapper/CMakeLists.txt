target_sources(VulkanWrapperCoreLibrary PUBLIC
    3rd_party.h
    fwd.h
)

add_subdirectory(Utils)
add_subdirectory(Window)
add_subdirectory(Vulkan)
add_subdirectory(Image)
add_subdirectory(Pipeline)
add_subdirectory(RenderPass)
add_subdirectory(Command)
add_subdirectory(Synchronization)
add_subdirectory(Memory)
add_subdirectory(Descriptors)
add_subdirectory(Model)
add_subdirectory(RayTracing)
