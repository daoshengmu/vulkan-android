//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanMain.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <string>
#include <component/Material.h>

#include "vulkan_wrapper.h"
#include "utils.h"
#include "VulkanRenderer.h"
#include "Object.h"

static const char* kTAG = "01-Vulkan-Triangle";

VulkanRenderer gRenderer;

bool InitVulkan(android_app* app) {
  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  // 8 vertex
  // create vertex / index buffer
  const std::vector<float> vertexData = {
      // Front face
      -0.5, -0.5, 0.5,
      0.5, -0.5, 0.5,
      0.5, 0.5, 0.5,
      -0.5, 0.5, 0.5,

      // Back face
      -0.5, -0.5, 1.0,
      -0.5, 0.5, 1.0,
      0.5, 0.5, 1.0,
      0.5, -0.5, 1.0,


//    // Front face
//    -1.0, -1.0, 1.0,
//    1.0, -1.0, 1.0,
//    1.0, 1.0, 1.0,
//    -1.0, 1.0, 1.0,
//
//    // Back face
//    -1.0, -1.0, -1.0,
//    -1.0, 1.0, -1.0,
//    1.0, 1.0, -1.0,
//    1.0, -1.0, -1.0,

    // Top face
//    -1.0, 1.0, -1.0,
//    -1.0, 1.0, 1.0,
//    1.0, 1.0, 1.0,
//    1.0, 1.0, -1.0,
//
//    // Bottom face
//    -1.0, -1.0, -1.0,
//    1.0, -1.0, -1.0,
//    1.0, -1.0, 1.0,
//    -1.0, -1.0, 1.0,
//
//    // Right face
//    1.0, -1.0, -1.0,
//    1.0, 1.0, -1.0,
//    1.0, 1.0, 1.0,
//    1.0, -1.0, 1.0,
//
//    // Left face
//    -1.0, -1.0, -1.0,
//    -1.0, -1.0, 1.0,
//    -1.0, 1.0, 1.0,
//    -1.0, 1.0, -1.0,
  };

  const std::vector<uint16_t> indexData = {
      0, 1, 2,    0, 2, 3,
      4, 5, 6,    4, 6, 7,
      5, 3, 2,    5, 2, 6,
      4, 7, 0,    4, 1, 0,
      7, 4, 6,    7, 2, 1,
      4, 0, 4,    4, 3, 5
  };

  RenderSurface surf;
  gRenderer.CreateVertexBuffer(vertexData, surf);
  gRenderer.CreateIndexBuffer(indexData, surf);

  VkShaderModule vertexShader, fragmentShader;
  gRenderer.LoadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
  gRenderer.LoadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);
  gRenderer.CreateGraphicsPipeline(vertexShader, fragmentShader, surf);

  surf.mVertexCount = 8;
  surf.mInstanceCount = 12;
  surf.mIndexCount = 36;

  gRenderer.AddSurface(surf);
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