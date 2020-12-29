//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanMain.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <string>
#include <MathUtils.h>

#include "vulkan_wrapper.h"
#include "Utils.h"
#include "VulkanRenderer.h"
#include "Matrix4x4.h"

static const char* kTAG = "03-Vulkan-Projection";

VulkanRenderer gRenderer;
std::shared_ptr<RenderSurface> gSurf = std::make_shared<RenderSurface>();

bool InitVulkan(android_app* app) {
  using namespace gfx_math;

  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  // 8 vertex
  // create vertex / index buffer
  const std::vector<float> vertexData = {
      // Front face
      -0.5, -0.5, -0.5,
      0.5, -0.5, -0.5,
      0.5, 0.5, -0.5,
      -0.5, 0.5, -0.5,

      // Back face
      -0.5, -0.5, 0.5,
      -0.5, 0.5, 0.5,
      0.5, 0.5, 0.5,
      0.5, -0.5, 0.5,
  };

  const std::vector<uint16_t> indexData = {
      0, 1, 2,  0, 2, 3,
      4, 5, 6,  4, 6, 7,
      5, 3, 2,  5, 2, 6,
      4, 7, 0,  7, 1, 0,
      7, 6, 2,  7, 2, 1,
      0, 5, 4,  0, 3, 5
  };

  struct UniformBufferObject {
    Matrix4x4f mvpMtx;
  };

  gSurf->mVertexCount = 8;
  gSurf->mInstanceCount = 12;
  gSurf->mIndexCount = 36;
  gSurf->mItemSize = 3;

  gRenderer.CreateVertexBuffer(vertexData, gSurf);
  gRenderer.CreateIndexBuffer(indexData, gSurf);
  gRenderer.CreateUniformBuffer(sizeof(UniformBufferObject), gSurf);

  // CreateDescriptorSetLayout needs to be after CreateTextureFromFile and CreateUniformBuffer
  gRenderer.CreateDescriptorSetLayout(gSurf);
  gRenderer.CreateGraphicsPipeline("shaders/uniform.vert.spv",
                                   "shaders/uniform.frag.spv", gSurf);
  gRenderer.CreateDescriptorSet(sizeof(UniformBufferObject), gSurf);
  gSurf->mTransformMatrix.Translate(0, 0, -10);

  gRenderer.AddSurface(gSurf);
  gRenderer.ConstructRenderPass();

  return true;
}

bool IsVulkanReady() {
  return gRenderer.IsReady();
}

void TerminateVulkan() {
  gRenderer.Terminate();
}

bool VulkanRenderFrame() {
  gSurf->mTransformMatrix.RotateY(DegreesToRadians(3.0f));
  gRenderer.RenderFrame();
  return true;
}