# ChronoFlow Vulkan compute integration (experimental)

The v0.4 Vulkan backend provides a **real Vulkan compute pipeline recording path**.
It does not create a swapchain, submit to a queue, capture games, measure latency, or
make any universal FPS guarantee. The host emulator/port must own those operations.

## Inputs & formats
- previous/current: VK_FORMAT_R8G8B8A8_UNORM sampled color images.
- flow: VK_FORMAT_R32G32_SFLOAT, current-to-previous displacement in PIXELS
  in current-frame space. Negative X means object moved right.
- confidence: VK_FORMAT_R32_SFLOAT, per-pixel motion confidence [0,1].
- output: VK_FORMAT_R8G8B8A8_UNORM STORAGE image in GENERAL layout.
- All images must use identical dimensions with appropriate image usage and format capabilities.
- Command buffer must be recording outside a render pass on a compute-capable queue.

## Building
```bash
cmake -S . -B build -DCF_ENABLE_VULKAN=ON -DCF_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
When glslangValidator is present CMake builds `interpolate.comp.spv`. Load
it as 32-bit SPIR-V words and pass them to `cf_vk_interpolator_create()`.
Vulkan headers/loader are provided by the Android NDK.

## Render integration and synchronization
1. Host produces TWO distinct real frames plus motion and confidence textures.
2. Host transitions inputs to SHADER_READ_ONLY_OPTIMAL, output to GENERAL,
   and inserts barriers for prior color/transfer writes.
3. Host creates interpolator using its own VkDevice and frame slots. Each
   slot must be idle (use fences) before recording a new job into it.
4. Host records `cf_vk_interpolator_record()` in a command buffer outside
   a render pass. It updates descriptors, binds a compute pipeline and dispatches.
5. Host inserts compute SHADER_WRITE -> transfer/sampling READ barrier,
   copies/composites output to its own swapchain, submits and presents.
6. Wait for all GPU uses before destroying interpolator resources.

## Limits
- Bidirectional interpolation needs a FUTURE frame and can add latency.
- Motion flow needs to be supplied. No depth-aware disocclusion, optical flow
  estimator, frame capture/injection or GPU timing implementation is provided.
- UI should be composited after interpolation; transparent regions/edges may ghost.
- Color interpolates in image encoding space; HDR/linear pipeline is future work.
- This module cannot fix emulator CPU bottlenecks or simulation clock bugs.
- Vulkan backend not yet runtime-tested on an Android GPU; use validation
  layers, Android GPU Inspector and device-specific measurements.

Sources:
- https://docs.vulkan.org/guide/latest/compute_shaders.html
- https://docs.vulkan.org/guide/latest/synchronization_examples.html
- https://github.khronos.org/Vulkan-Site/tutorial/latest/Advanced_Vulkan_Compute/12_Mobile_and_Embedded_Compute/02_android_compute.html
