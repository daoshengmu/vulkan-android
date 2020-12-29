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
#include "Cube.h"
#include "Matrix4x4.h"

static const char* kTAG = "04-Vulkan-TextureMapping";

VulkanRenderer gRenderer;
std::shared_ptr<Cube> gSurf = std::make_shared<Cube>();

bool InitVulkan(android_app* app) {
  using namespace gfx_math;

  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  struct UniformBufferObject {
    Matrix4x4f mvpMtx;
  };

  gRenderer.CreateVertexBuffer(gSurf->GetVertexData(), gSurf);
  gRenderer.CreateIndexBuffer(gSurf->GetIndexData(), gSurf);
  gRenderer.CreateUniformBuffer(sizeof(UniformBufferObject), gSurf);
  gRenderer.CreateTextureFromFile("assets/textures/crate01_color_height_rgba.ktx", gSurf);

  // CreateDescriptorSetLayout needs to be after CreateTextureFromFile and CreateUniformBuffer
  gRenderer.CreateDescriptorSetLayout(gSurf);
  // Call DescriptorSetLayout must before CreateGraphicsPipeline
  gRenderer.CreateGraphicsPipeline("shaders/texture.vert.spv",
                                   "shaders/texture.frag.spv", gSurf);
  gRenderer.CreateDescriptorSet(sizeof(UniformBufferObject), gSurf);
  gSurf->mTransformMatrix.Translate(0, 0, -15);

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