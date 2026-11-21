# ChronoFlow Universal — SDK v0.4 (experimental)

ChronoFlow is an open-source C++17 library designed for developers to **integrate
into their own emulator, Android port or game engine**. It is NOT an app that
injects code into closed-source games.

## Current status
- Portable C ABI and C++17 performance policy, FPS/time management and host callbacks.
- **Tested CPU reference frame interpolation** using backward motion vectors.
- **Experimental Vulkan compute integration** for host-provided images and flow;
  actual device validation and performance measurements are still pending.
- CMake and CI builds with optional Vulkan, portable tests and a C example.

## What it does NOT yet do
No standalone frame-generation APK, universal emulator compatibility, automatic
optical-flow estimation, swapchain integration, input latency reduction, or
proven GPU performance gains. Interpolation is not an engine speedup and
does not correct slow emulation clocks on its own.

## Build & tests
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
For Vulkan: `cmake -S . -B build -DCF_ENABLE_VULKAN=ON`.
Host must supply Vulkan SDK/Android NDK and glslangValidator to compile GLSL shader.

## Integrate
- [Host SDK](include/chronoflow/chronoflow.h)
- [CPU interpolator API](include/chronoflow/reprojection.h)
- [Vulkan bridge API](include/chronoflow/vulkan_bridge.h)
- [Vulkan integration contract](docs/VULKAN.md)
- [Host integration](docs/INTEGRATION.md)
- [C example](examples/integration_example.c)

**License:** MIT. Previous v0.1/v0.2 experiments are research references;
this branch implements a portable API and GPU path, not a production driver.
