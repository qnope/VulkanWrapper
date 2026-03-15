# Task Dependencies

## Dependency Graph

```
Task 1: Primitive Library (no deps)
    │
    ├──► Task 2: Common Scene Setup (depends on Task 1)
    │        │
    │        ├──► Task 4: CubeShadow (depends on Task 1, 2)
    │        │
    │        ├──► Task 5: AmbientOcclusion (depends on Task 1, 2)
    │        │
    │        └──► Task 6: EmissiveCube (depends on Task 1, 2)
    │
    └──► Task 3: Triangle (depends on Task 1 only)
```

## Execution Order

### Phase 1: Foundation
- **Task 1: Primitive Library** — Must be completed first. All other tasks depend on it.

### Phase 2: Infrastructure + Simple Example (parallelizable)
- **Task 2: Common Scene Setup** — Can start once Task 1 is done.
- **Task 3: Triangle Example** — Can start once Task 1 is done. Independent of Task 2.

### Phase 3: Deferred Examples (parallelizable)
- **Task 4: CubeShadow** — Can start once Tasks 1 + 2 are done.
- **Task 5: AmbientOcclusion** — Can start once Tasks 1 + 2 are done.
- **Task 6: EmissiveCube** — Can start once Tasks 1 + 2 are done.

Tasks 4, 5, and 6 are independent of each other and can be implemented in parallel.

## Recommended Implementation Order

For a single developer, the suggested order is:

1. **Task 1** — Primitive Library (foundation + tests)
2. **Task 2** — Common Scene Setup (shared helpers)
3. **Task 3** — Triangle (simplest, validates Primitive + build system)
4. **Task 4** — CubeShadow (first deferred example, validates Common setup)
5. **Task 5** — AmbientOcclusion (adds AO + blit pattern)
6. **Task 6** — EmissiveCube (adds emissive material + full pipeline)

## CMake Registration Summary

After all tasks, `examples/CMakeLists.txt` should contain:

```cmake
add_subdirectory(Common)
add_subdirectory(Application)
add_subdirectory(Advanced)
add_subdirectory(Triangle)
add_subdirectory(CubeShadow)
add_subdirectory(AmbientOcclusion)
add_subdirectory(EmissiveCube)
```

And `VulkanWrapper/tests/CMakeLists.txt` gains the `PrimitiveTests` executable.
