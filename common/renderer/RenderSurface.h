//
// Created by Daosheng Mu on 12/13/20.
//

#ifndef VULKANANDROID_RENDERSURFACE_H
#define VULKANANDROID_RENDERSURFACE_H

#include "Matrix4x4.h"

using namespace gfx_math;

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

class RenderSurface {

public:
  int mVertexCount = 0;
  int mInstanceCount = 0;
  int mFirstVertex = 0;
  int mFirstInstance = 0;
  int mIndexCount = 0;
  Matrix4x4f  mTransformMatrix;
  int mUBOSize = 0;

private:
  struct VulkanBufferInfo {
    VkBuffer vertexBuf;
    VkBuffer indexBuf = VK_NULL_HANDLE;
  };

  struct VulkanGfxPipelineInfo {
    VkPipelineLayout layout;
    VkPipelineCache cache;
    VkPipeline pipeline;
  };

  // buffer
  std::vector<float> mVertexData;
  std::vector<uint16_t> mIndexData;
  VulkanBufferInfo mBuffer;
  VulkanGfxPipelineInfo mGfxPipeline;
  VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE; // TODO: clean it up
  VkDescriptorPool mDescriptorPool; // TODO: clean it up
  std::vector<VkDescriptorSet> mDescriptorSets; // TODO: clean it up
  std::vector<VkBuffer> mUniformBuffers;
  std::vector<VkDeviceMemory> mUniformBuffersMemory;

  // material

  //  texture

  // transformNode

  friend class VulkanRenderer;
};

#endif //VULKAN_RENDERSURFACE_H
