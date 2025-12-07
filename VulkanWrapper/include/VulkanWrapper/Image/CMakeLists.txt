target_sources(VulkanWrapperCoreLibrary PUBLIC
    Image.h
    ImageView.h
    Sampler.h
    CombinedImage.h
    Mipmap.h
)

if(TARGET SDL3_image::SDL3_image)
    target_sources(VulkanWrapperCoreLibrary PUBLIC ImageLoader.h)
endif()
