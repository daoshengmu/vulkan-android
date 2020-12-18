//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanMain.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <vector>

#include "vulkan_wrapper.h"
#include "utils.h"
#include "VulkanRenderer.h"

static const char* kTAG = "01-Vulkan-Triangle";

VulkanRenderer gRenderer;

bool InitVulkan(android_app* app) {
  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  // create vertex / index buffer
  const std::vector<float> vertexData = {
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
  };
  RenderSurface surf;
  gRenderer.CreateVertexBuffer(vertexData, surf);

  // 1. instantiate a surface
  // 2. create vertex / index buffer
  // 3. create shader
  VkShaderModule vertexShader, fragmentShader;
  gRenderer.LoadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
  gRenderer.LoadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);
  gRenderer.CreateGraphicsPipeline(vertexShader, fragmentShader, surf);
  // We don't need the shaders anymore, we can release their memory
  gRenderer.DestroyShaderModule(vertexShader);
  gRenderer.DestroyShaderModule(fragmentShader);

  surf.mVertexCount = 3;
  surf.mInstanceCount = 1;

  // 4. create textures

  gRenderer.AddSurface(surf);

  // create command buffer
  gRenderer.CreateCommandBuffer();

  return true;
}

bool IsVulkanReady() {
  return gRenderer.IsReady();
}

void TerminateVulkan() {
  gRenderer.Terminate();
}

bool VulkanRenderFrame() {
  gRenderer.RenderFrame();
  return true;
}