#ifndef CHRONOFLOW_VULKAN_BRIDGE_H
#define CHRONOFLOW_VULKAN_BRIDGE_H
/* Optional Vulkan 1.0 compute backend. The host owns device, images, command
 * buffers, queue submissions and ALL layout transitions and synchronization. */
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cf_vk_interpolator cf_vk_interpolator;
typedef struct cf_vk_create_info {
  VkDevice device;
  uint32_t frames_in_flight; /* >0; host must wait for per-slot fence */
  const uint32_t *shader_spirv_words;
  size_t shader_spirv_size_bytes; /* precompiled shaders/interpolate.comp */
} cf_vk_create_info;
typedef struct cf_vk_frame {
  VkCommandBuffer command_buffer; /* active recording; outside render pass */
  uint32_t frame_slot;            /* must be idle before reusing descriptor */
  VkImageView previous_rgba8;      /* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */
  VkImageView current_rgba8;       /* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */
  VkImageView flow_rg32f;         /* current->previous flow in PIXELS */
  VkImageView confidence_r32f;    /* per-pixel confidence in [0,1] */
  VkImageView output_rgba8;       /* VK_IMAGE_LAYOUT_GENERAL */
  uint32_t width;
  uint32_t height;
  float alpha;                    /* interpolation factor [0,1] */
  float minimum_confidence;       /* [0,1] */
  float max_flow_pixels;          /* >0 */
} cf_vk_frame;
VkResult cf_vk_interpolator_create(const cf_vk_create_info *,cf_vk_interpolator **);
VkResult cf_vk_interpolator_record(cf_vk_interpolator *,const cf_vk_frame *);
void cf_vk_interpolator_destroy(cf_vk_interpolator *);
#ifdef __cplusplus
}
#endif
#endif
