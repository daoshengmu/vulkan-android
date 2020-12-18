//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKANTRIANGLE_VULKANRENDERER_H
#define VULKANTRIANGLE_VULKANRENDERER_H

#include <string>
#include <vector>
#include <vector>
#include "vulkan_wrapper.h"
#include "RenderSurface.h"

struct android_app;
class ANativeWindow;
class VkApplicationInfo;

class VulkanRenderer {
public:
  VulkanRenderer() : mAppContext(nullptr), mInitialized(false) {}
  bool Init(android_app* app, const std::string& aAppName);
  bool IsReady();
  void Terminate();
  void RenderFrame();
//  bool AddObject(const Object& aObj);
  bool AddSurface(const RenderSurface& aSurf);
  void CreateVertexBuffer(const std::vector<float>& aVertexData, RenderSurface& aSurf);
  void CreateIndexBuffer(const std::vector<uint16_t>& aIndexData, RenderSurface& aSurf);
  void CreateCommandBuffer();
  VkResult CreateGraphicsPipeline(VkShaderModule aVertexShader,
                                  VkShaderModule aFragmentShader,
                                  RenderSurface& aSurf);
  VkResult LoadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
                              ShaderType type);
  void DestroyShaderModule(VkShaderModule aShader);

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

  // TODO: remove it.
//  struct VulkanBufferInfo {
//    VkBuffer vertexBuf;
//  };

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
//
//  enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

  bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask,
                            uint32_t* typeIndex);
  void CreateVulkanDevice(ANativeWindow* platformWindow,
                          VkApplicationInfo* appInfo);
  void CreateSwapChain();
  void CreateFrameBuffers(VkRenderPass& renderPass,
                          VkImageView depthView = VK_NULL_HANDLE);
//  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
//                    VkBuffer& buffer, VkDeviceMemory& bufferMemory);
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
 // VulkanBufferInfo mBuffer;
//  VulkanGfxPipelineInfo mGfxPipeline;

//  std::vector<Object> mObjects;
  // TODO: should we use shared_ptr?
  std::vector<RenderSurface> mSurfaces;
  bool mInitialized;
};


#endif //VULKANTRIANGLE_VULKANRENDERER_H
