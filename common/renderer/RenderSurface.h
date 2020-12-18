//
// Created by Daosheng Mu on 12/13/20.
//

#ifndef VULKANTRIANGLE_RENDERSURFACE_H
#define VULKANTRIANGLE_RENDERSURFACE_H

//#include "VulkanRenderer.h"

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

class RenderSurface {

public:
  int mVertexCount = 0;
  int mInstanceCount = 0;
  int mFirstVertex = 0;
  int mFirstInstance = 0;

  int mIndexCount = 0;

private:
  struct VulkanBufferInfo {
    VkBuffer vertexBuf;
    VkBuffer indexBuf = 0;
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

  // material
  //  shader
 // VkShaderModule mVertexShader;
 // VkShaderModule mFragShader;

  //  texture

  // transformNode

  friend class VulkanRenderer;
};

#endif //VULKANTRIANGLE_RENDERSURFACE_H
