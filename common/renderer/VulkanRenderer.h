//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKANANDROID_RENDERER_H
#define VULKANANDROID_RENDERER_H

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
  bool CreateTextureFromFile(const char* aFilePath, std::shared_ptr<RenderSurface> aSurf);
  void CreateDescriptorSetLayout(std::shared_ptr<RenderSurface> aSurf);
  void CreateDescriptorSet(VkDeviceSize aBufferSize, std::shared_ptr<RenderSurface> aSurf);
  VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin);
  void ConstructRenderPass();
  void FlushCommandBuffer(VkCommandBuffer aCommandBuffer, VkQueue aQueue, bool free, VkSemaphore aSignalSemaphore = VK_NULL_HANDLE);
  VkResult CreateGraphicsPipeline(const char* aVSPath, const char* aFSPath, std::shared_ptr<RenderSurface> aSurf);

private:

  struct VulkanPhysicalDeviceFeature {
    VkBool32  samplerAnisotropy;
  };

  struct VulkanDeviceInfo {
    VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice gpuDevice;
    // We didn't copy all attributes of VkPhysicalDeviceFeatures,
    // just the attributes we need to save the memory of the struct.
    VulkanPhysicalDeviceFeature gpuDeviceFeatures;
    VkPhysicalDeviceProperties gpuDeviceProperties;
    VkDevice device;
    uint32_t queueFamilyIndex;

    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentqueue;
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
  void CreateCommandPool();
  void CreateDescriptorPool(std::shared_ptr<RenderSurface> aSurf);
  void CreateSyncObjects();
  void CreateCommandBuffer();
  VkResult LoadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
                              ShaderType type);
  void SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);
  void SetupPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& aFeatures);
  void UpdateUniformBuffer(int aImageIndex);
  void DeleteSwapChain();
  void DeleteGraphicsPipeline();
  void DeleteTextures();
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

  bool mInitialized;
};

#endif //VULKANANDROID_RENDERER_H
