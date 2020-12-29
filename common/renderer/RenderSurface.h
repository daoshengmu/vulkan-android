//
// Created by Daosheng Mu on 12/13/20.
//

#ifndef VULKANANDROID_RENDERSURFACE_H
#define VULKANANDROID_RENDERSURFACE_H

#include "Matrix4x4.h"
#include "vulkan_wrapper.h"

using namespace gfx_math;

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

class RenderSurface {

public:
  enum VertexInputType {
    VertexInputType_Pos3,
    VertexInputType_Pos3Color4Normal3UV2,
  };

  int mVertexCount = 0;
  int mInstanceCount = 0;
  int mFirstVertex = 0;
  int mFirstInstance = 0;
  int mItemSize = 0;
  int mIndexCount = 0;
  int mUBOSize = 0;
  VertexInputType mVertexInput = VertexInputType_Pos3;
  Matrix4x4f  mTransformMatrix;

private:
  struct VulkanBufferInfo {
    VkBuffer vertexBuf = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufMemory = VK_NULL_HANDLE;
    VkBuffer indexBuf = VK_NULL_HANDLE;
    VkDeviceMemory indexBufMemory = VK_NULL_HANDLE;
  };

  struct VulkanGfxPipelineInfo {
    VkPipelineLayout layout;
    VkPipelineCache cache;
    VkPipeline pipeline;
  };

  struct VulkanTexture {
    VkSampler      sampler;
    VkImage        image;
    VkImageLayout  imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView    view;
    uint32_t       width;
    uint32_t       height;
    uint32_t       mipLevels;
    VkFormat       format;
  };

  // buffer
  std::vector<float> mVertexData;
  std::vector<uint16_t> mIndexData;
  VulkanBufferInfo mBuffer; // it includes vertex and index buffers.
  VulkanGfxPipelineInfo mGfxPipeline;
  VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool;
  std::vector<VkDescriptorSet> mDescriptorSets;
  std::vector<VkBuffer> mUniformBuffers;
  std::vector<VkDeviceMemory> mUniformBuffersMemory;
  std::vector<VulkanTexture> mTextures;

  // material

  //  texture

  // transformNode

  friend class VulkanRenderer;
};

#endif //VULKAN_RENDERSURFACE_H
