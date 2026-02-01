---
sidebar_position: 3
---

# Shader Binding Table

The Shader Binding Table (SBT) defines which shaders are invoked during ray tracing.

## Overview

```cpp
#include <VulkanWrapper/RayTracing/ShaderBindingTable.h>

// Create SBT from pipeline
auto sbt = pipeline->create_sbt(allocator);

// Use in traceRaysKHR
cmd.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    sbt.callable_region(),
    width, height, 1
);
```

## SBT Structure

The SBT is organized into regions:

```
┌─────────────────────┐
│   Ray Generation    │  Single entry
├─────────────────────┤
│      Miss 0         │
│      Miss 1         │  Miss shader entries
│        ...          │
├─────────────────────┤
│    Hit Group 0      │
│    Hit Group 1      │  Hit group entries
│        ...          │
├─────────────────────┤
│    Callable 0       │  Optional callable
│        ...          │  shader entries
└─────────────────────┘
```

## ShaderBindingTable Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `raygen_region()` | `vk::StridedDeviceAddressRegionKHR` | Ray gen shader region |
| `miss_region()` | `vk::StridedDeviceAddressRegionKHR` | Miss shaders region |
| `hit_region()` | `vk::StridedDeviceAddressRegionKHR` | Hit groups region |
| `callable_region()` | `vk::StridedDeviceAddressRegionKHR` | Callable shaders region |

### Region Structure

```cpp
struct vk::StridedDeviceAddressRegionKHR {
    vk::DeviceAddress deviceAddress;  // Buffer address
    vk::DeviceSize stride;            // Entry stride
    vk::DeviceSize size;              // Total size
};
```

## Creating the SBT

### From Pipeline

```cpp
// Automatic creation
auto sbt = pipeline->create_sbt(allocator);
```

### Manual Creation

```cpp
// Query shader group handles
auto handleSize = rtProperties.shaderGroupHandleSize;
auto handleAlignment = rtProperties.shaderGroupHandleAlignment;
auto baseAlignment = rtProperties.shaderGroupBaseAlignment;

// Calculate entry sizes (aligned)
auto alignedHandleSize = align(handleSize, handleAlignment);
auto raygenSize = align(alignedHandleSize, baseAlignment);
auto missSize = align(alignedHandleSize * missCount, baseAlignment);
auto hitSize = align(alignedHandleSize * hitGroupCount, baseAlignment);

// Allocate buffer
auto sbtBuffer = allocator->allocate<SBTBuffer>(
    raygenSize + missSize + hitSize
);

// Write handles...
```

## Shader Group Indices

When building the pipeline, shader groups are assigned indices:

```cpp
// Index 0: Ray generation
builder.addRaygenShader(raygen);

// Index 1, 2: Miss shaders
builder.addMissShader(primaryMiss);   // Index 1
builder.addMissShader(shadowMiss);    // Index 2

// Index 3, 4: Hit groups
builder.addHitGroup(triangleHit);     // Index 3
builder.addHitGroup(proceduralHit);   // Index 4
```

## Using Multiple Miss Shaders

Select miss shader in traceRayEXT:

```glsl
// Primary ray - use miss shader 0
traceRayEXT(tlas, flags, 0xFF,
            0,    // SBT offset
            0,    // SBT stride
            0,    // Miss index (uses miss shader 0)
            origin, tMin, direction, tMax, 0);

// Shadow ray - use miss shader 1
traceRayEXT(tlas, flags, 0xFF,
            0, 0,
            1,    // Miss index (uses miss shader 1)
            origin, tMin, direction, tMax, 1);
```

## Using Multiple Hit Groups

Select hit group based on instance and geometry:

```glsl
// SBT offset = instanceOffset + geometryOffset
// instanceOffset = instance.instanceShaderBindingTableRecordOffset
// geometryOffset = geometryIndex * SBT record stride

traceRayEXT(tlas, flags, 0xFF,
            0,    // SBT offset (added to instance offset)
            1,    // SBT stride (for multiple geometries)
            0,    // Miss index
            origin, tMin, direction, tMax, 0);
```

### Instance Configuration

```cpp
vk::AccelerationStructureInstanceKHR instance{
    .transform = transform,
    .instanceCustomIndex = 0,
    .mask = 0xFF,
    .instanceShaderBindingTableRecordOffset = 0,  // Hit group index
    .accelerationStructureReference = blasAddress
};
```

## Shader Record Data

Custom data can be embedded in SBT entries:

```cpp
struct HitShaderRecord {
    uint8_t handle[32];  // Shader group handle
    uint32_t materialId; // Custom data
    uint32_t textureId;
};
```

Access in shader:
```glsl
layout(shaderRecordEXT) buffer ShaderRecord {
    uint materialId;
    uint textureId;
} record;
```

## Example: Shadow Rays

```cpp
// Pipeline with two miss shaders
builder.addMissShader(primaryMiss);  // Returns sky color
builder.addMissShader(shadowMiss);   // Returns "not in shadow"

// In closest hit shader:
void main() {
    // Compute lighting...

    // Trace shadow ray
    isShadowed = true;
    traceRayEXT(tlas,
                gl_RayFlagsTerminateOnFirstHitEXT |
                gl_RayFlagsSkipClosestHitShaderEXT,
                0xFF,
                0, 0,
                1,  // Shadow miss shader
                hitPoint, 0.001, lightDir, lightDist,
                1); // Shadow payload

    if (!isShadowed) {
        // Add light contribution
    }
}
```

## Best Practices

1. **Order miss shaders** by frequency of use
2. **Use instance offset** for per-instance hit groups
3. **Minimize SBT size** by sharing hit groups
4. **Consider shader record data** for material IDs
5. **Align entries** to hardware requirements
