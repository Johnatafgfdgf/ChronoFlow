#include "chronoflow/vulkan_bridge.h"
#include <array>
#include <cmath>
#include <new>
#include <vector>
#include <cstdint>

struct cf_vk_interpolator {
  VkDevice device=VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_layout=VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;
  VkPipeline pipeline=VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool=VK_NULL_HANDLE;
  VkSampler linear_sampler=VK_NULL_HANDLE;
  VkSampler nearest_sampler=VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptors;
};

extern "C" void cf_vk_interpolator_destroy(cf_vk_interpolator *p) {
  if(!p)return;
  if(p->device!=VK_NULL_HANDLE) {
    // Host MUST ensure GPU is finished using this object's resources first.
    if(p->pipeline)vkDestroyPipeline(p->device,p->pipeline,nullptr);
    if(p->pipeline_layout)vkDestroyPipelineLayout(p->device,p->pipeline_layout,nullptr);
    if(p->descriptor_pool)vkDestroyDescriptorPool(p->device,p->descriptor_pool,nullptr);
    if(p->descriptor_layout)vkDestroyDescriptorSetLayout(p->device,p->descriptor_layout,nullptr);
    if(p->linear_sampler)vkDestroySampler(p->device,p->linear_sampler,nullptr);
    if(p->nearest_sampler)vkDestroySampler(p->device,p->nearest_sampler,nullptr);
  }
  delete p;
}

extern "C" VkResult cf_vk_interpolator_create(const cf_vk_create_info *info,cf_vk_interpolator **out) {
  if(!out)return VK_ERROR_INITIALIZATION_FAILED;
  *out=nullptr;
  if(!info||!info->device||!info->shader_spirv_words||
     info->shader_spirv_size_bytes<4||info->shader_spirv_size_bytes%4!=0||
     info->frames_in_flight==0||info->frames_in_flight>16)return VK_ERROR_INITIALIZATION_FAILED;
  auto *p=new(std::nothrow) cf_vk_interpolator{};
  if(!p)return VK_ERROR_OUT_OF_HOST_MEMORY;
  p->device=info->device;
  VkResult r=VK_SUCCESS;
  do {
    std::array<VkDescriptorSetLayoutBinding,5> bindings{};
    for(uint32_t i=0;i<5;++i) {
      bindings[i].binding=i;
      bindings[i].descriptorCount=1;
      bindings[i].descriptorType=(i==4)?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo dci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dci.bindingCount=static_cast<uint32_t>(bindings.size());dci.pBindings=bindings.data();
    r=vkCreateDescriptorSetLayout(p->device,&dci,nullptr,&p->descriptor_layout);
    if(r!=VK_SUCCESS)break;
    VkPushConstantRange pc{};
    pc.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;pc.offset=0;pc.size=20;
    VkPipelineLayoutCreateInfo lci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    lci.setLayoutCount=1;lci.pSetLayouts=&p->descriptor_layout;
    lci.pushConstantRangeCount=1;lci.pPushConstantRanges=&pc;
    r=vkCreatePipelineLayout(p->device,&lci,nullptr,&p->pipeline_layout);
    if(r!=VK_SUCCESS)break;
    VkShaderModule module=VK_NULL_HANDLE;
    VkShaderModuleCreateInfo sci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    sci.codeSize=info->shader_spirv_size_bytes;sci.pCode=info->shader_spirv_words;
    r=vkCreateShaderModule(p->device,&sci,nullptr,&module);
    if(r!=VK_SUCCESS)break;
    VkComputePipelineCreateInfo pci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pci.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pci.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
    pci.stage.module=module;pci.stage.pName="main";
    pci.layout=p->pipeline_layout;
    r=vkCreateComputePipelines(p->device,VK_NULL_HANDLE,1,&pci,nullptr,&p->pipeline);
    vkDestroyShaderModule(p->device,module,nullptr);
    if(r!=VK_SUCCESS)break;
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.minFilter=VK_FILTER_LINEAR; sampler.magFilter=VK_FILTER_LINEAR;
    sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxLod=0.0f;
    r=vkCreateSampler(p->device,&sampler,nullptr,&p->linear_sampler);
    if(r!=VK_SUCCESS)break;
    sampler.minFilter=VK_FILTER_NEAREST;sampler.magFilter=VK_FILTER_NEAREST;
    r=vkCreateSampler(p->device,&sampler,nullptr,&p->nearest_sampler);
    if(r!=VK_SUCCESS)break;
    std::array<VkDescriptorPoolSize,2> sizes{{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,info->frames_in_flight*4},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,info->frames_in_flight}}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets=info->frames_in_flight;pool.poolSizeCount=2;pool.pPoolSizes=sizes.data();
    r=vkCreateDescriptorPool(p->device,&pool,nullptr,&p->descriptor_pool);
    if(r!=VK_SUCCESS)break;
    std::vector<VkDescriptorSetLayout> layouts(info->frames_in_flight,p->descriptor_layout);
    p->descriptors.resize(info->frames_in_flight,VK_NULL_HANDLE);
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool=p->descriptor_pool;
    ai.descriptorSetCount=info->frames_in_flight;
    ai.pSetLayouts=layouts.data();
    r=vkAllocateDescriptorSets(p->device,&ai,p->descriptors.data());
  }while(false);
  if(r!=VK_SUCCESS){cf_vk_interpolator_destroy(p);return r;}
  *out=p;return VK_SUCCESS;
}

extern "C" VkResult cf_vk_interpolator_record(cf_vk_interpolator *p,const cf_vk_frame *f) {
  if(!p||!f||!f->command_buffer||f->frame_slot>=p->descriptors.size()||
     !f->previous_rgba8||!f->current_rgba8||!f->flow_rg32f||
     !f->confidence_r32f||!f->output_rgba8||!f->width||!f->height||
     !std::isfinite(f->alpha)||f->alpha<0.f||f->alpha>1.f||
     !std::isfinite(f->minimum_confidence)||f->minimum_confidence<0.f||f->minimum_confidence>1.f||
     !std::isfinite(f->max_flow_pixels)||f->max_flow_pixels<=0.f ||
     f->width>65535*8u||f->height>65535*8u)
    return VK_ERROR_INITIALIZATION_FAILED;

  // Synchronization, layout transitions and lifetime all belong to the host.
  VkDescriptorSet set=p->descriptors[f->frame_slot];
  std::array<VkImageView,5> views{{f->previous_rgba8,f->current_rgba8,
                                   f->flow_rg32f,f->confidence_r32f,f->output_rgba8}};
  std::array<VkDescriptorImageInfo,5> images{};
  std::array<VkWriteDescriptorSet,5> writes{};
  for(uint32_t i=0;i<5;++i) {
    images[i].imageView=views[i];
    images[i].imageLayout=i==4?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if(i<4)images[i].sampler=i<2?p->linear_sampler:p->nearest_sampler;
    writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet=set;
    writes[i].dstBinding=i;
    writes[i].descriptorCount=1;
    writes[i].descriptorType=i==4?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[i].pImageInfo=&images[i];
  }
  vkUpdateDescriptorSets(p->device,5,writes.data(),0,nullptr);
  struct Push {
    float alpha,minimumConfidence,maxFlowPixels;
    uint32_t width,height;
  } push{f->alpha,f->minimum_confidence,f->max_flow_pixels,f->width,f->height};
  static_assert(sizeof(Push)==20,"push constant size mismatch");
  vkCmdBindPipeline(f->command_buffer,VK_PIPELINE_BIND_POINT_COMPUTE,p->pipeline);
  vkCmdBindDescriptorSets(f->command_buffer,VK_PIPELINE_BIND_POINT_COMPUTE,
                          p->pipeline_layout,0,1,&set,0,nullptr);
  vkCmdPushConstants(f->command_buffer,p->pipeline_layout,
                     VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),&push);
  vkCmdDispatch(f->command_buffer,(f->width+7)/8,(f->height+7)/8,1);
  return VK_SUCCESS; // command recording is not a guarantee of successful GPU execution
}
