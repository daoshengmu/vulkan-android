//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanMain.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <string>
#include <component/Material.h>

#include "vulkan_wrapper.h"
#include "VulkanRenderer.h"

static const char* kTAG = "02-Vulkan-Cube-IndexBuffer";

VulkanRenderer gRenderer;

bool InitVulkan(android_app* app) {
  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  // 8 vertex
  // create vertex / index buffer
  const std::vector<float> vertexData = {
      // Front face
      -0.5, -0.5, 0.0,
      0.5, -0.5, 0.0,
      0.5, 0.5, 0.0,
      -0.5, 0.5, 0.0,

      // Back face
      -0.5, -0.5, 1.0,
      -0.5, 0.5, 1.0,
      0.5, 0.5, 1.0,
      0.5, -0.5, 1.0,
  };

  const std::vector<uint16_t> indexData = {
      0, 1, 2,  0, 2, 3,
      4, 5, 6,  4, 6, 7,
      5, 3, 2,  5, 2, 6,
      4, 7, 0,  7, 1, 0,
      7, 6, 2,  7, 2, 1,
      0, 5, 4,  0, 3, 5
  };

  auto surf = std::make_shared<RenderSurface>();
  surf->mVertexCount = 8;
  surf->mInstanceCount = 12;
  surf->mIndexCount = 36;
  surf->mItemSize = 3;

  gRenderer.CreateVertexBuffer(vertexData, surf);
  gRenderer.CreateIndexBuffer(indexData, surf);
  gRenderer.CreateGraphicsPipeline("shaders/tri.vert.spv",
          "shaders/tri.frag.spv", surf);

  gRenderer.AddSurface(surf);
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
  gRenderer.RenderFrame();
  return true;
}