# CLAUDE.md

You can work on your side until you finish your implementation and build / tests pass
When it's finished make a patch and let me review your code step by step so it is easy for me to review
Never finish if tests or build does not pass

## Project Overview

VulkanWrapper is a C++23 library that provides a modern, high-level abstraction over the Vulkan graphics API. The library wraps Vulkan's verbose C-style API with RAII-based objects, automatic resource management, and simplified interfaces for common operations.

**Key Features:**
- Modern C++23 interface with concepts and ranges
- RAII-based resource management using Vulkan-Hpp unique handles
- Automatic memory management via Vulkan Memory Allocator (VMA)
- Ray tracing pipeline support (BLAS/TLAS, shader binding tables)
- Resource state tracking and automatic barrier insertion
- Shader compilation integration with glslangValidator
- Cross-language bindings (C API in `vw_c/`, Python in `vw_py/`, Rust in `vw_rust/`)

## Build System

**Build tool:** CMake 3.25+ with vcpkg for dependencies

**Build configurations:**

```bash
# Build
cmake --build build-Clang20Debug
```

## Tests
```bash
# run all tests
ctest

# run specific tests
ctest -R <name_of_the_test>
```

tests are located in VulkanWrapper/tests
