---
sidebar_position: 1
---

# Instance

The `Instance` class represents a Vulkan instance, which is the connection between your application and the Vulkan library.

## Overview

```cpp
#include <VulkanWrapper/Vulkan/Instance.h>

auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .build();
```

## InstanceBuilder

Use `InstanceBuilder` to configure and create an instance:

### Methods

| Method | Description |
|--------|-------------|
| `setDebug()` | Enable validation layers and debug messenger |
| `setApiVersion(ApiVersion)` | Set Vulkan API version (e10, e11, e12, e13) |
| `addPortability()` | Enable portability enumeration (for MoltenVK) |
| `addExtension(const char*)` | Add a single extension |
| `addExtensions(span<const char* const>)` | Add multiple extensions |
| `build()` | Create the instance |

### Example

```cpp
// Create instance with window extensions
auto instance = InstanceBuilder()
    .setDebug()                              // Validation layers
    .setApiVersion(ApiVersion::e13)          // Vulkan 1.3
    .addPortability()                        // macOS support
    .addExtensions(window->vulkan_extensions())
    .build();
```

## Instance Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::Instance` | Get the raw Vulkan handle |
| `findGpu()` | `DeviceFinder` | Start GPU selection process |

### Finding a GPU

```cpp
auto device = instance->findGpu()
    .requireGraphicsQueue()
    .requirePresentQueue(surface)
    .requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    .find()
    .build();
```

## API Version

The `ApiVersion` enum specifies the Vulkan version:

| Value | Version |
|-------|---------|
| `e10` | Vulkan 1.0 |
| `e11` | Vulkan 1.1 |
| `e12` | Vulkan 1.2 |
| `e13` | Vulkan 1.3 (recommended) |

VulkanWrapper is designed for Vulkan 1.3 and uses modern features like:
- Dynamic rendering (no render pass objects)
- Synchronization2 (simplified barriers)
- Maintenance4 (various improvements)

## Validation Layers

When `setDebug()` is called:
- `VK_LAYER_KHRONOS_validation` is enabled
- Debug messenger captures validation messages
- Messages are printed to stderr

```cpp
// Debug output example:
// VALIDATION: vkCreateGraphicsPipelines: required parameter
//             pCreateInfos[0].pStages[0].module was not valid
```

## Extensions

Common extensions you might need:

```cpp
// Surface extensions (usually from window)
instance->addExtensions(window->vulkan_extensions());

// Portability (MoltenVK on macOS)
instance->addPortability();

// Debug utils (added automatically with setDebug())
instance->addExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
```

## Lifetime

The instance must outlive all objects created from it:

```cpp
{
    auto instance = InstanceBuilder().setDebug().build();
    auto device = instance->findGpu()./* ... */.build();

    // Use device...

}  // device destroyed first, then instance
```

Using `std::shared_ptr` ensures proper destruction order.
