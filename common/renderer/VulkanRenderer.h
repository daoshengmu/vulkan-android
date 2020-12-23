//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <string>
#include <vector>
#include <memory>
#include "vulkan_wrapper.h"
#include "RenderSurface.h"
#include "Matrix4x4.h"

struct android_app;
class ANativeWindow;
class VkApplicationInfo;

using namespace gfx_math;

class VulkanRenderer {
public:
  VulkanRenderer() : mAppContext(nullptr), mInitialized(false) {}
  bool Init(android_app* app, const std::string& aAppName);
 // void SetupViewport(int aX, int aY, int aWidth, int aHeight, float aNear, float aFar);
  bool IsReady();
  void Terminate();
  void RenderFrame();
  bool AddSurface(std::shared_ptr<RenderSurface> aSurf);
  void CreateVertexBuffer(const std::vector<float>& aVertexData, std::shared_ptr<RenderSurface> aSurf);
  void CreateIndexBuffer(const std::vector<uint16_t>& aIndexData, std::shared_ptr<RenderSurface> aSurf);
  void CreateUniformBuffer(VkDeviceSize aBufferSize, std::shared_ptr<RenderSurface> aSurf);
  void CreateCommandBuffer();
  VkResult CreateGraphicsPipeline(const char* aVSPath, const char* aFSPath, std::shared_ptr<RenderSurface> aSurf);

private:
  struct VulkanDeviceInfo {
    VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice gpuDevice;
    VkDevice device;
    uint32_t queueFamilyIndex;

    VkSurfaceKHR surface;
    VkQueue queue;
  };

  struct VulkanSwapchainInfo {
    VkSwapchainKHR swapchain;
    uint32_t swapchainLength;

    VkExtent2D displaySize;
    VkFormat displayFormat;

    // array of frame buffers and views
    std::vector<VkImage> displayImages;
    std::vector<VkImageView> displayViews;
    std::vector<VkFramebuffer> framebuffers;
  };

  struct VulkanRenderInfo {
    VkRenderPass renderPass;
    VkCommandPool cmdPool;
    VkCommandBuffer* cmdBuffer;
    uint32_t cmdBufferLen;
    VkSemaphore semaphore;
    VkFence fence;
  };

//  struct VulkanGfxPipelineInfo {
//    VkPipelineLayout layout;
//    VkPipelineCache cache;
//    VkPipeline pipeline;
//  };

  bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask,
                            uint32_t* typeIndex);
  void CreateVulkanDevice(ANativeWindow* platformWindow,
                          VkApplicationInfo* appInfo);
  void CreateSwapChain();
  void CreateFrameBuffers(VkRenderPass& renderPass,
                          VkImageView depthView = VK_NULL_HANDLE);
  VkResult LoadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
                              ShaderType type);
  void SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);
  void UpdateUniformBuffer(int aImageIndex);
  void DeleteSwapChain();
  void DeleteGraphicsPipeline();
  void DeleteBuffers();
  void DeleteDescriptors();
  void DestroyShaderModule(VkShaderModule aShader);

  android_app* mAppContext;
  VulkanDeviceInfo mDeviceInfo;
  VulkanSwapchainInfo mSwapchain;
  VulkanRenderInfo mRenderInfo;

  std::vector<std::shared_ptr<RenderSurface>> mSurfaces;
  Matrix4x4f mViewMatrix;
  Matrix4x4f mProjMatrix;
  //Rect viewPort;
//  float viewNear;
//  float viewFar;

  bool mInitialized;
};


#endif //VULKAN_RENDERER_H
