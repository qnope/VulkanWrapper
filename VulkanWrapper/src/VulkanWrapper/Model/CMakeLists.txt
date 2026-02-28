target_sources(VulkanWrapperCoreLibrary PRIVATE
    Importer.cpp
    Mesh.cpp
    MeshManager.cpp
    MeshRenderer.cpp
)

add_subdirectory(Material)
add_subdirectory(Internal)
