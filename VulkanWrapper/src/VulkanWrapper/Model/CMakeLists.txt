target_sources(VulkanWrapperCoreLibrary PRIVATE
    Importer.cpp
    Mesh.cpp
    MeshManager.cpp
)

add_subdirectory(Material)
add_subdirectory(Internal)
