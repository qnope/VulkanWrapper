target_sources(3DRenderer PRIVATE
    3DRenderer/3rd_party.h

    3DRenderer/Core/fwd.h
    3DRenderer/Core/Utils/Algos.h
    3DRenderer/Core/Utils/exceptions.h

    3DRenderer/Core/Window/SDL_Initializer.h
    3DRenderer/Core/Window/Window.h

    3DRenderer/Core/Vulkan/ObjectWithHandle.h
    3DRenderer/Core/Vulkan/Instance.h
    3DRenderer/Core/Vulkan/Device.h
    3DRenderer/Core/Vulkan/Queue.h
    3DRenderer/Core/Vulkan/PresentQueue.h
    3DRenderer/Core/Vulkan/DeviceFinder.h
    3DRenderer/Core/Vulkan/PhysicalDevice.h
)
