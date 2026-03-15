# Dependency Graph

## Task Dependencies

```
Task 1: Create ExampleRunner + Update Common CMake
   ├── Task 2: Rewrite Triangle        (depends on Task 1)
   ├── Task 3: Rewrite CubeShadow      (depends on Task 1)
   ├── Task 4: Rewrite AmbientOcclusion (depends on Task 1)
   └── Task 5: Rewrite EmissiveCube     (depends on Task 1)
```

## Execution Order

1. **Task 1** must be completed first (creates the base class that all others depend on)
2. **Tasks 2-5** are independent of each other and can be done in any order or in parallel

## Build Dependencies

```
VulkanWrapper::VW  ←  App (STATIC)  ←  ExamplesCommon (STATIC, includes ExampleRunner)
                                            ↑
                              Triangle, CubeShadow, AmbientOcclusion, EmissiveCube
```

- `ExamplesCommon` depends on `App` (PUBLIC) and `VulkanWrapper::VW` (PUBLIC)
- All four example executables depend only on `ExamplesCommon` (which transitively provides App and VW)
- The **Advanced** example is NOT affected by this project

## Validation Order

After all tasks are complete:
1. Build all targets: `cmake --build build`
2. Run each example and verify screenshot output
3. Compare screenshots with pre-refactor baselines
