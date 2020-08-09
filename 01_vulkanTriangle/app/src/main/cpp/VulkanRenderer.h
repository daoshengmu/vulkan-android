//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKANTRIANGLE_VULKANRENDERER_H
#define VULKANTRIANGLE_VULKANRENDERER_H

#include <string>
#include <vector>
#include "vulkan_wrapper.h"

struct android_app;
class ANativeWindow;
class VkApplicationInfo;

class VulkanRenderer {
public:
  VulkanRenderer() : mAppContext(nullptr), mInitialized(false) {}
  bool Init(android_app* app, std::string aAppName);
  bool IsReady();
  void Terminate();
  void RenderFrame();

private:
  struct VulkanDeviceInfo {
    VkInstance instance;
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

  struct VulkanBufferInfo {
    VkBuffer vertexBuf;
  };

  struct VulkanRenderInfo {
    VkRenderPass renderPass;
    VkCommandPool cmdPool;
    VkCommandBuffer* cmdBuffer;
    uint32_t cmdBufferLen;
    VkSemaphore semaphore;
    VkFence fence;
  };

  struct VulkanGfxPipelineInfo {
    VkPipelineLayout layout;
    VkPipelineCache cache;
    VkPipeline pipeline;
  };

  enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

  bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask,
                            uint32_t* typeIndex);
  void CreateVulkanDevice(ANativeWindow* platformWindow,
                          VkApplicationInfo* appInfo);
  void CreateSwapChain();
  void CreateFrameBuffers(VkRenderPass& renderPass,
                          VkImageView depthView = VK_NULL_HANDLE);
  void CreateBuffers();
  VkResult CreateGraphicsPipeline();
  VkResult loadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
                              ShaderType type);
  void SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                      VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                      VkPipelineStageFlags srcStages,
                      VkPipelineStageFlags destStages);
  void DeleteSwapChain();
  void DeleteGraphicsPipeline();
  void DeleteBuffers();

  android_app* mAppContext;
  VulkanDeviceInfo mDeviceInfo;
  VulkanSwapchainInfo mSwapchain;
  VulkanRenderInfo mRenderInfo;
  VulkanBufferInfo mBuffer;
  VulkanGfxPipelineInfo mGfxPipeline;

  bool mInitialized;
};


#endif //VULKANTRIANGLE_VULKANRENDERER_H
