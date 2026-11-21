#ifndef CHRONOFLOW_REPROJECTION_H
#define CHRONOFLOW_REPROJECTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Reference CPU frame interpolation. Input and output pixel layout: RGBA8, row-major.
 * Flow is CURRENT -> PREVIOUS in PIXELS, two floats/pixel, row-major;
 * both inputs must be from the same frame size and must not overlap output.
 * The optional confidence image is one float/pixel in [0,1].
 * Low confidence/invalid flow falls back to the current frame at that pixel.
 * alpha=0 -> previous; alpha=1 -> current. Output intermediate needs both
 * real frames, so introduces at least one frame of lookahead latency.
 */
typedef struct cf_rgba8_image {
  const uint8_t *data;
  uint32_t width;
  uint32_t height;
  uint32_t stride_bytes; /* >= width*4 */
} cf_rgba8_image;
typedef struct cf_reprojection_settings {
  float alpha;             /* 0..1 */
  float min_confidence;    /* 0..1 */
  float max_flow_pixels;   /* >0, guard unexpected velocity */
} cf_reprojection_settings;
typedef enum cf_reprojection_result {
  CF_REPROJECTION_OK=0,
  CF_REPROJECTION_INVALID=1
} cf_reprojection_result;
cf_reprojection_result cf_interpolate_rgba8(
  cf_rgba8_image previous,
  cf_rgba8_image current,
  const float *current_to_previous_xy,
  const float *confidence_or_null,
  cf_reprojection_settings settings,
  uint8_t *output_rgba8,
  uint32_t output_stride_bytes);
#ifdef __cplusplus
}
#endif
#endif
