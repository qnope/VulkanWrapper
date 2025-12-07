target_sources(VulkanWrapperCoreLibrary PRIVATE
    Image.cpp
    ImageView.cpp
    Sampler.cpp
    CombinedImage.cpp
    Mipmap.cpp
)

if(TARGET SDL3_image::SDL3_image)
    target_sources(VulkanWrapperCoreLibrary PRIVATE ImageLoader.cpp)
endif()
