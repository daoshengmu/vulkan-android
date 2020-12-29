//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanRenderer.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include "ktx.h"
#include "vulkan_wrapper.h"
#include "Utils.h"
#include "Platform.h"
#include "MathUtils.h"

static std::string gAppName;

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, gAppName.c_str(),          \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type,
        uint64_t object, size_t location, int32_t message_code,
        const char *layer_prefix, const char *message, void *user_data) {
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    LOG_E("Validation Layer: Error: {}: {}", layer_prefix, message);
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    LOG_E("Validation Layer: Warning: {}: {}", layer_prefix, message);
  }
  else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
  {
    LOG_I("Validation Layer: Performance warning: {}: {}", layer_prefix, message);
  }
  else
  {
    LOG_I("Validation Layer: Information: {}: {}", layer_prefix, message);
  }
  return VK_FALSE;
}

const std::vector<VkVertexInputAttributeDescription>&
GetVertexInputAttributeDescription(RenderSurface::VertexInputType aType) {
  switch (aType) {
    case RenderSurface::VertexInputType_Pos3:
      const static std::vector<VkVertexInputAttributeDescription> vertexPos = {
        {
          .binding = 0,
          .location = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0
        }
      };
      return vertexPos;
    case RenderSurface::VertexInputType_Pos3Color4Normal3UV2:
      const static std::vector<VkVertexInputAttributeDescription> vertexPosColorNormalUV = {
        {
          .binding = 0,
          .location = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0
        },
        {
          .binding = 0,
          .location = 1,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
          .offset = 3 * sizeof(float)
        },
        {
          .binding = 0,
          .location = 2,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 7 * sizeof(float)
        },
        {
          .binding = 0,
          .location = 3,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 10 * sizeof(float)
        }
      };
      return vertexPosColorNormalUV;
    default:
      const static std::vector<VkVertexInputAttributeDescription> undefined;
      LOG_E(gAppName.data(), "Undefined VertexInputType.");
      assert(false);
      return undefined;
  }
}

void VulkanRenderer::CreateVulkanDevice(ANativeWindow* platformWindow,
                                        VkApplicationInfo* appInfo) {
  LOG_I(gAppName.c_str(), "CreateVulkanDevice");
  std::vector<const char*> instance_extensions;
  std::vector<const char*> device_extensions;

  instance_extensions.push_back("VK_KHR_surface");
  // TODO: Support other platforms instead of only Android.
  instance_extensions.push_back("VK_KHR_android_surface");

  if (enableValidationLayers) {
    instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }

  device_extensions.push_back("VK_KHR_swapchain");

  // Create the Vulkan instance
  VkInstanceCreateInfo instanceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = nullptr,
    .pApplicationInfo = appInfo,
    .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
    .ppEnabledExtensionNames = instance_extensions.data(),
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = nullptr,
  };

  // Print out all available layers.
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (const auto& layerProperties : availableLayers) {
    LOG_I(gAppName.c_str(), "Available Vulkan layers: %s", layerProperties.layerName);
  }

  // Create the validation layer
  VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo;
  if (enableValidationLayers) {
    debugReportCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
            .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
            .pfnCallback = debugReportCallback,
    };
    instanceCreateInfo.pNext = &debugReportCreateInfo;
  }

  CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &mDeviceInfo.instance));
  VulkanLoadInstance(mDeviceInfo.instance);

  // Create the validation layer
  if (enableValidationLayers) {
    CALL_VK(vkCreateDebugReportCallbackEXT(mDeviceInfo.instance, &debugReportCreateInfo, nullptr,
            &mDeviceInfo.debugReportCallback));
  }

  VkAndroidSurfaceCreateInfoKHR createInfo{
          .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
          .pNext = nullptr,
          .flags = 0,
          .window = platformWindow
  };
  CALL_VK(vkCreateAndroidSurfaceKHR(mDeviceInfo.instance, &createInfo, nullptr,
                                          &mDeviceInfo.surface));

  // Find one GPU to use:
  // On Android, every GPU device is equal -- supporting
  // graphics/compute/present
  // for this sample, we use the very first GPU device found on the system
  // Emulators can't get an available GPU device, so need to run it on a
  // physical device.
  uint32_t gpuCount = 0;
  CALL_VK(vkEnumeratePhysicalDevices(mDeviceInfo.instance, &gpuCount, nullptr));
  assert(gpuCount);
  VkPhysicalDevice tmpGpus[gpuCount];
  CALL_VK(vkEnumeratePhysicalDevices(mDeviceInfo.instance, &gpuCount, tmpGpus));
  // Pick up the first GPU Device
  mDeviceInfo.gpuDevice = tmpGpus[0];

  // Find a GFX queue family
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(mDeviceInfo.gpuDevice, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(mDeviceInfo.gpuDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());
  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(mDeviceInfo.gpuDevice, &supportedFeatures);
  SetupPhysicalDeviceFeatures(supportedFeatures);
  vkGetPhysicalDeviceProperties(mDeviceInfo.gpuDevice, &mDeviceInfo.gpuDeviceProperties);

  uint32_t queueFamilyIndex;
  for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
       queueFamilyIndex++) {
    if (queueFamilyProperties[queueFamilyIndex].queueFlags &
        VK_QUEUE_GRAPHICS_BIT) {
      break;
    }
  }
  assert(queueFamilyIndex < queueFamilyCount);
  mDeviceInfo.queueFamilyIndex = queueFamilyIndex;

  // Create a logical device (Vulkan device)
  float priorities[] = {1.0f};
  VkDeviceQueueCreateInfo queueCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueCount = 1,
    .queueFamilyIndex = queueFamilyIndex,
    .pQueuePriorities = priorities,
  };

  VkDeviceCreateInfo deviceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueCreateInfo,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
    .ppEnabledExtensionNames = device_extensions.data(),
    .pEnabledFeatures = nullptr,
  };

  CALL_VK(vkCreateDevice(mDeviceInfo.gpuDevice, &deviceCreateInfo, nullptr,
                               &mDeviceInfo.device));
  vkGetDeviceQueue(mDeviceInfo.device, mDeviceInfo.queueFamilyIndex, 0,
                   &mDeviceInfo.presentqueue);
  vkGetDeviceQueue(mDeviceInfo.device, mDeviceInfo.queueFamilyIndex, 0,
                   &mDeviceInfo.graphicsQueue);
}

void VulkanRenderer::CreateSwapChain() {
  LOG_I(gAppName.c_str(), "CreateSwapChain");
  memset(&mSwapchain, 0, sizeof(mSwapchain));

  // Get the surface capabilities because:
  //   - It contains the minimal and max length of the chain, we will need it
  //   - It's necessary to query the supported surface format (R8G8B8A8 for
  //   instance ...)
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDeviceInfo.gpuDevice, mDeviceInfo.surface,
                                            &surfaceCapabilities);
  // Query the list of supported surface format and choose one we like
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(mDeviceInfo.gpuDevice, mDeviceInfo.surface,
                                       &formatCount, nullptr);
  VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[formatCount];
  vkGetPhysicalDeviceSurfaceFormatsKHR(mDeviceInfo.gpuDevice, mDeviceInfo.surface,
                                       &formatCount, formats);
  LOG_I(gAppName.c_str(), "Got %d formats", formatCount);

  // Find out R8G8B8A8 format swapchain.
  uint32_t chosenFormat;
  for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
    if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) {
      break;
    }
  }
  assert(chosenFormat < formatCount);

  mSwapchain.displaySize = surfaceCapabilities.currentExtent;
  mSwapchain.displayFormat = formats[chosenFormat].format;

  // Create a swap chain (here we choose the minimum available number of surface
  // in the chain)
  VkSwapchainCreateInfoKHR swapchainCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .surface = mDeviceInfo.surface,
    .minImageCount = surfaceCapabilities.minImageCount,
    .imageFormat = formats[chosenFormat].format,
    .imageColorSpace = formats[chosenFormat].colorSpace,
    .imageExtent = surfaceCapabilities.currentExtent,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .imageArrayLayers = 1,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &mDeviceInfo.queueFamilyIndex,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .oldSwapchain = VK_NULL_HANDLE,
    .clipped = VK_FALSE,
  };
  CALL_VK(vkCreateSwapchainKHR(mDeviceInfo.device, &swapchainCreateInfo, nullptr,
                                     &mSwapchain.swapchain));

  // Get the length of the created swap chain
  CALL_VK(vkGetSwapchainImagesKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                        &mSwapchain.swapchainLength, nullptr));
  delete[] formats;
}

bool VulkanRenderer::Init(android_app* app, const std::string& aAppName) {
  mAppContext = app;
  gAppName = aAppName;

  if (!InitVulkan()) {
    LOG_W(gAppName.c_str(), "Vulkan is unavailable, install vulkan and re-start");
    return false;
  }

  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .apiVersion = VK_MAKE_VERSION(1, 0, 0),
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .pApplicationName = gAppName.c_str(),
    .pEngineName = "tutorial",
  };

  // create a device
  CreateVulkanDevice(app->window, &appInfo);

  // create swapchain
  CreateSwapChain();

  // create a render pass
  VkAttachmentDescription attachmentDescriptions{
    .format = mSwapchain.displayFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference colourReference = {
    .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDescription{
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .flags = 0,
    .inputAttachmentCount = 0,
    .pInputAttachments = nullptr,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colourReference,
    .pResolveAttachments = nullptr,
    .pDepthStencilAttachment = nullptr,
    .preserveAttachmentCount = 0,
    .pPreserveAttachments = nullptr,
  };
  VkRenderPassCreateInfo renderPassCreateInfo{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = nullptr,
    .attachmentCount = 1,
    .pAttachments = &attachmentDescriptions,
    .subpassCount = 1,
    .pSubpasses = &subpassDescription,
    .dependencyCount = 0,
    .pDependencies = nullptr,
  };
  CALL_VK(vkCreateRenderPass(mDeviceInfo.device, &renderPassCreateInfo, nullptr,
                                   &mRenderInfo.renderPass));

  // Create double frame buffers.
  CreateFrameBuffers(mRenderInfo.renderPass);
  CreateCommandPool();

  // Setup view and projection matrix.
  mViewMatrix = Matrix4x4f::LookAtMatrix(Vector3Df(0,0,-5),
                                    Vector3Df(0,0,-100), Vector3Df(0, 1, 0));
  mProjMatrix = Matrix4x4f::Perspective(DegreesToRadians(60.0f), (float)mSwapchain.displaySize.width / mSwapchain.displaySize.height,
                                       0.001f, 256.0f);
  // gfx_math Matrix was originally designed for OpenGL,
  // where the Y coordinate of the clip coordinates is inverted with Vulkan.
  mProjMatrix._11 *= -1.0f;
  mInitialized = true;
  return true;
}

bool VulkanRenderer::MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask,
                                          uint32_t* typeIndex) {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(mDeviceInfo.gpuDevice, &memoryProperties);
  // Search memtypes to find first index with those properties
  for (uint32_t i = 0; i < 32; i++) {
    if ((typeBits & 1) == 1) {
      // Type is available, does it match user properties?
      if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) ==
          requirements_mask) {
        *typeIndex = i;
        return true;
      }
    }
    typeBits >>= 1;
  }
  return false;
}

VkResult VulkanRenderer::LoadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
                                            ShaderType type) {
  // Read the file
  assert(mAppContext);
  AAsset* file = AAssetManager_open(mAppContext->activity->assetManager,
                                    filePath, AASSET_MODE_BUFFER);
  size_t fileLength = AAsset_getLength(file);
  assert(fileLength);
  char* fileContent = new char[fileLength];

  AAsset_read(file, fileContent, fileLength);
  AAsset_close(file);

  VkShaderModuleCreateInfo shaderModuleCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .codeSize = fileLength,
    .pCode = (const uint32_t*)fileContent,
    .flags = 0,
  };
  VkResult result = vkCreateShaderModule(mDeviceInfo.device,
                                         &shaderModuleCreateInfo,
                                         nullptr, shaderOut);
  assert(result == VK_SUCCESS);
  delete[] fileContent;

  return result;
}

void VulkanRenderer::SetImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                                    VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                                    VkPipelineStageFlags srcStages,
                                    VkPipelineStageFlags destStages) {
  VkImageMemoryBarrier imageMemoryBarrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = NULL,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
    .oldLayout = oldImageLayout,
    .newLayout = newImageLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };

  switch (oldImageLayout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;
    default:
      break;
  }

  switch (newImageLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.dstAccessMask =
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    default:
      break;
  }

  vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1,
                       &imageMemoryBarrier);
}

void VulkanRenderer::SetupPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& aFeatures) {
  mDeviceInfo.gpuDeviceFeatures.samplerAnisotropy = aFeatures.samplerAnisotropy;
}

void VulkanRenderer::UpdateUniformBuffer(int aImageIndex) {
  for (const auto& surf : mSurfaces) {
    if (!surf->mUBOSize) {
      continue;
    }

    void* data;
    vkMapMemory(mDeviceInfo.device, surf->mUniformBuffersMemory[aImageIndex], 0, surf->mUBOSize, 0, &data);

    Matrix4x4f mvpMtx;
    mvpMtx = mProjMatrix * mViewMatrix * surf->mTransformMatrix;

    // TODO: support other uniform data, we currently only has the requirement of the MVP mtx.
    memcpy(data, &mvpMtx, surf->mUBOSize);
    vkUnmapMemory(mDeviceInfo.device, surf->mUniformBuffersMemory[aImageIndex]);
  }
}

void VulkanRenderer::CreateCommandPool() {
  // Create a pool of command buffers to allocate command buffer from
  VkCommandPoolCreateInfo cmdPoolCreateInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .pNext = nullptr,
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .queueFamilyIndex = mDeviceInfo.queueFamilyIndex,
  };
  CALL_VK(vkCreateCommandPool(mDeviceInfo.device, &cmdPoolCreateInfo, nullptr,
                              &mRenderInfo.cmdPool));
}

void VulkanRenderer::CreateFrameBuffers(VkRenderPass& renderPass,
                                        VkImageView depthView) {
  // query display attachment to swapchain
  uint32_t swapchainImagesCount = 0;
  CALL_VK(vkGetSwapchainImagesKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                        &swapchainImagesCount, nullptr));
  mSwapchain.displayImages.resize(swapchainImagesCount);
  CALL_VK(vkGetSwapchainImagesKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                        &swapchainImagesCount,
                                        mSwapchain.displayImages.data()));

  // create image view for each swapchain image
  mSwapchain.displayViews.resize(swapchainImagesCount);
  for (uint32_t i = 0; i < swapchainImagesCount; i++) {
    VkImageViewCreateInfo viewCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .image = mSwapchain.displayImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = mSwapchain.displayFormat,
      .components =
      {
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
      },
      .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .flags = 0,
    };
    CALL_VK(vkCreateImageView(mDeviceInfo.device, &viewCreateInfo, nullptr,
                                    &mSwapchain.displayViews[i]));
  }
  // create a framebuffer from each swapchain image
  mSwapchain.framebuffers.resize(mSwapchain.swapchainLength);
  for (uint32_t i = 0; i < mSwapchain.swapchainLength; i++) {
    VkImageView attachments[2] = {
      mSwapchain.displayViews[i], depthView, // depthView is NULL by default
    };
    VkFramebufferCreateInfo fbCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .renderPass = renderPass,
            .layers = 1,
            .attachmentCount = 1,  // 2 if using depth
            .pAttachments = attachments,
            .width = static_cast<uint32_t>(mSwapchain.displaySize.width),
            .height = static_cast<uint32_t>(mSwapchain.displaySize.height),
    };
    fbCreateInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2);

    CALL_VK(vkCreateFramebuffer(mDeviceInfo.device, &fbCreateInfo, nullptr,
                                &mSwapchain.framebuffers[i]));
  }
}

void VulkanRenderer::CreateSyncObjects() {
  // We need to create a fence to be able, in the main loop, to wait for our
  // draw command(s) to finish before swapping the framebuffers
  VkFenceCreateInfo fenceCreateInfo{
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
  };
  CALL_VK(vkCreateFence(mDeviceInfo.device, &fenceCreateInfo,
                        nullptr, &mRenderInfo.fence));

  // We need to create a semaphore to be able to wait, in the main loop, for our
  // framebuffer to be available for us before drawing.
  VkSemaphoreCreateInfo semaphoreCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0
  };
  CALL_VK(vkCreateSemaphore(mDeviceInfo.device, &semaphoreCreateInfo, nullptr,
                            &mRenderInfo.semaphore));
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = mRenderInfo.cmdPool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(mDeviceInfo.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer aCommandBuffer) {
  vkEndCommandBuffer(aCommandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &aCommandBuffer;

  // TODO: what is the difference between vkWaitForFences and vkQueueWaitIdle?
  vkQueueSubmit(mDeviceInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(mDeviceInfo.graphicsQueue);

  vkFreeCommandBuffers(mDeviceInfo.device, mRenderInfo.cmdPool, 1, &aCommandBuffer);
}

void VulkanRenderer::CopyBuffer(VkBuffer aSrcBuffer, VkBuffer aDstBuffer, VkDeviceSize aSize) {
  VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = aSize;
  vkCmdCopyBuffer(commandBuffer, aSrcBuffer, aDstBuffer, 1, &copyRegion);

  EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CreateBuffer(VkDeviceSize aSize, VkBufferUsageFlags aUsage,
                                  VkMemoryPropertyFlags aProperties, VkBuffer& aBuffer,
                                  VkDeviceMemory& aBufferMemory) {
  // Create a index buffer
  VkBufferCreateInfo createBufferInfo{
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .pNext = nullptr,
          .size = aSize,
          .usage = aUsage,
          .flags = 0,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .pQueueFamilyIndices = &mDeviceInfo.queueFamilyIndex,
          .queueFamilyIndexCount = 1,
  };

  CALL_VK(vkCreateBuffer(mDeviceInfo.device, &createBufferInfo, nullptr,
                         &aBuffer));

  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(mDeviceInfo.device, aBuffer, &memReq);

  VkMemoryAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          .pNext = nullptr,
          .allocationSize = memReq.size,
          .memoryTypeIndex = 0,  // Memory type assigned in the next step
  };

  // Assign the proper memory type for that buffer
  MapMemoryTypeToIndex(memReq.memoryTypeBits, aProperties, &allocInfo.memoryTypeIndex);
  // Allocate memory for the buffer
  CALL_VK(vkAllocateMemory(mDeviceInfo.device, &allocInfo, nullptr, &aBufferMemory));
  CALL_VK(vkBindBufferMemory(mDeviceInfo.device, aBuffer, aBufferMemory, 0));
}

void VulkanRenderer::CreateUniformBuffer(VkDeviceSize aBufferSize,
                                         std::shared_ptr<RenderSurface> aSurf) {
  const int swapchainCount = mSwapchain.displayImages.size();
  aSurf->mUniformBuffers.resize(swapchainCount);
  aSurf->mUniformBuffersMemory.resize(swapchainCount);
  aSurf->mUBOSize = aBufferSize;

  // Create uniform buffers. We don't need a staging buffer.
  // We will update the buffers every frame.
  for (size_t i = 0; i < swapchainCount; i++) {
    CreateBuffer(aBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            aSurf->mUniformBuffers[i], aSurf->mUniformBuffersMemory[i]);
  }
}

void VulkanRenderer::CreateVertexBuffer(const std::vector<float>& aVertexData,
                                        std::shared_ptr<RenderSurface> aSurf) {
  aSurf->mVertexData = aVertexData;
  const size_t bufferSize = aVertexData.size() * sizeof(float);
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  // Create a staging buffer as the src buffer for letting data copy on it.
  CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void* data;
  CALL_VK(vkMapMemory(mDeviceInfo.device, stagingBufferMemory, 0, bufferSize,
                      0, &data));
  memcpy(data, aVertexData.data(), bufferSize);
  vkUnmapMemory(mDeviceInfo.device, stagingBufferMemory);

  // Create a local buffer and let staging buffer copy on it for the GPU optimal usage.
  // (It might be acceptable just copying data to the local buffer without staging buffer
  // and it would have penalty on performance.
  CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aSurf->mBuffer.vertexBuf, aSurf->mBuffer.vertexBufMemory);
  CopyBuffer(stagingBuffer, aSurf->mBuffer.vertexBuf, bufferSize);

  vkDestroyBuffer(mDeviceInfo.device, stagingBuffer, nullptr);
  vkFreeMemory(mDeviceInfo.device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::CreateIndexBuffer(const std::vector<uint16_t>& aIndexData,
                                       std::shared_ptr<RenderSurface> aSurf) {

  aSurf->mIndexData = aIndexData;
  const size_t bufferSize = aIndexData.size() * sizeof(uint16_t);
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  // Create a staging buffer as the src buffer for letting data copy on it.
  CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          stagingBuffer, stagingBufferMemory);

  void* data;
  CALL_VK(vkMapMemory(mDeviceInfo.device, stagingBufferMemory, 0, bufferSize,
                      0, &data));
  memcpy(data, aIndexData.data(), bufferSize);
  vkUnmapMemory(mDeviceInfo.device, stagingBufferMemory);

  // Create a local buffer and let staging buffer copy on it for the GPU optimal usage.
  // (It might be acceptable just copying data to the local buffer without staging buffer
  // and it would have penalty on performance.
  CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aSurf->mBuffer.indexBuf, aSurf->mBuffer.indexBufMemory);
  CopyBuffer(stagingBuffer, aSurf->mBuffer.indexBuf, bufferSize);

  vkDestroyBuffer(mDeviceInfo.device, stagingBuffer, nullptr);
  vkFreeMemory(mDeviceInfo.device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::CreateDescriptorSetLayout(std::shared_ptr<RenderSurface> aSurf) {
  std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

  if (aSurf->mUniformBuffers.size()) {
    layoutBindings.push_back(
      // create ubo descriptor layout
      {
        .binding = 0, // the binding index of vertex shader.
        // the amount of items of this layout, ex: for the case of a bone matrix, it will not be just one.
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImmutableSamplers = nullptr,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, // TODO: it needs to be adapted for FRAGMENT_BIT.
      }
    );
  }

  if (aSurf->mTextures.size()) {
    layoutBindings.push_back(
      {
        .binding = 1, // the binding index of fragment shader.
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = nullptr,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      }
    );
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
    .pBindings = layoutBindings.data(),
  };

  CALL_VK(vkCreateDescriptorSetLayout(mDeviceInfo.device, &layoutInfo, nullptr,
                                      &aSurf->mDescriptorSetLayout));
}

VkResult VulkanRenderer::CreateGraphicsPipeline(const char* aVSPath,
                                                const char* aFSPath,
                                                std::shared_ptr<RenderSurface> aSurf) {
  memset(&aSurf->mGfxPipeline, 0, sizeof(RenderSurface::VulkanGfxPipelineInfo));

  VkShaderModule vertexShader, fragmentShader;
  LoadShaderFromFile(aVSPath, &vertexShader, VERTEX_SHADER);
  LoadShaderFromFile(aFSPath, &fragmentShader, FRAGMENT_SHADER);

  // TODO: The pipeline layout works for uniform buffers, we should create
  //  it in a RenderSurface and make description pool supports not only one uniform buffer.
  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .setLayoutCount = aSurf->mUniformBuffers.size() == 0 ? (uint32_t)0 : 1,
    .pSetLayouts = aSurf->mUniformBuffers.size() == 0 ? nullptr : &aSurf->mDescriptorSetLayout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr,
  };

  CALL_VK(vkCreatePipelineLayout(mDeviceInfo.device, &pipelineLayoutCreateInfo,
                                       nullptr, &aSurf->mGfxPipeline.layout));

  // No dynamic state in that tutorial
  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = nullptr,
    .dynamicStateCount = 0,
    .pDynamicStates = nullptr
  };

  // Specify vertex and fragment shader stages
  VkPipelineShaderStageCreateInfo shaderStages[2]{
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertexShader,
      .pSpecializationInfo = nullptr,
      .flags = 0,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragmentShader,
      .pSpecializationInfo = nullptr,
      .flags = 0,
      .pName = "main",
    }
  };

  VkViewport viewports{
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
    .x = 0,
    .y = 0,
    .width = (float)mSwapchain.displaySize.width,
    .height = (float)mSwapchain.displaySize.height,
  };

  VkRect2D scissor = {
    .extent = mSwapchain.displaySize,
    .offset = {
      .x = 0, .y = 0
  }};
  // Specify viewport info
  VkPipelineViewportStateCreateInfo viewportInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .viewportCount = 1,
    .pViewports = &viewports,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  // Specify multisample info
  VkSampleMask sampleMask = ~0u;
  VkPipelineMultisampleStateCreateInfo multisampleInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 0,
    .pSampleMask = &sampleMask,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };

  // Specify color blend state (disable it)
  VkPipelineColorBlendAttachmentState attachmentStates{
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
  };
  VkPipelineColorBlendStateCreateInfo colorBlendInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &attachmentStates,
    .flags = 0,
  };

  // Specify rasterizer info
  VkPipelineRasterizationStateCreateInfo rasterInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1,
  };

  // Specify input assembler state
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  // Specify vertex input state
  VkVertexInputBindingDescription vertex_input_bindings{
    .binding = 0,
    .stride = aSurf->mItemSize * uint32_t(sizeof(float)),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  const auto& vertexInput = GetVertexInputAttributeDescription(aSurf->mVertexInput);
//  VkVertexInputAttributeDescription vertex_input_attributes[2]{{ // from vertex shader
//      .binding = 0,
//      .location = 0,
//      .format = VK_FORMAT_R32G32B32_SFLOAT,
//      .offset = 0
//    }, {
//      .binding = 0,
//      .location = 1,
//      .format = VK_FORMAT_R32G32B32_SFLOAT,
//      .offset = 3 * 4 // 4: float
//    }
//  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertex_input_bindings,
//    .vertexAttributeDescriptionCount = sizeof(vertex_input_attributes)
//                                    / sizeof(VkVertexInputAttributeDescription),
//    .pVertexAttributeDescriptions = vertex_input_attributes
    .vertexAttributeDescriptionCount = (uint32_t)vertexInput.size(),
    .pVertexAttributeDescriptions = vertexInput.data()
  };

  // Create the pipeline cache
  VkPipelineCacheCreateInfo pipelineCacheInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    .pNext = nullptr,
    .initialDataSize = 0,
    .pInitialData = nullptr,
    .flags = 0,  // reserved, must be 0
  };

  CALL_VK(vkCreatePipelineCache(mDeviceInfo.device, &pipelineCacheInfo, nullptr,
                                      &aSurf->mGfxPipeline.cache));

  // Create the pipeline
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssemblyInfo,
    .pTessellationState = nullptr,
    .pViewportState = &viewportInfo,
    .pRasterizationState = &rasterInfo,
    .pMultisampleState = &multisampleInfo,
    .pDepthStencilState = nullptr,
    .pColorBlendState = &colorBlendInfo,
    .pDynamicState = &dynamicStateInfo,
    .layout = aSurf->mGfxPipeline.layout,
    .renderPass = mRenderInfo.renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0,
  };

  VkResult pipelineResult = vkCreateGraphicsPipelines(
                              mDeviceInfo.device, aSurf->mGfxPipeline.cache, 1, &pipelineCreateInfo, nullptr,
                              &aSurf->mGfxPipeline.pipeline);
  DestroyShaderModule(vertexShader);
  DestroyShaderModule(fragmentShader);

  return pipelineResult;
}

void VulkanRenderer::CreateDescriptorPool(std::shared_ptr<RenderSurface> aSurf) {
  const int swapchainCount = mSwapchain.displayImages.size();
  std::vector<VkDescriptorPoolSize> poolSize;

  if (aSurf->mUniformBuffers.size()) {
    poolSize.push_back(
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(swapchainCount),
      }
    );
  }

  if (aSurf->mTextures.size()) {
    poolSize.push_back(
      {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(swapchainCount),
      }
    );
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSize.size();
  poolInfo.pPoolSizes = poolSize.data();
  poolInfo.maxSets = static_cast<uint32_t>(swapchainCount);

  CALL_VK(vkCreateDescriptorPool(mDeviceInfo.device, &poolInfo, nullptr, &aSurf->mDescriptorPool));
}

void VulkanRenderer::CreateDescriptorSet(VkDeviceSize aBufferSize, std::shared_ptr<RenderSurface> aSurf) {
  CreateDescriptorPool(aSurf);

  const int swapchainCount = mSwapchain.displayImages.size();
  std::vector<VkDescriptorSetLayout> layouts(swapchainCount, aSurf->mDescriptorSetLayout);
  VkDescriptorSetAllocateInfo desAllocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = aSurf->mDescriptorPool,
          .descriptorSetCount = static_cast<uint32_t>(swapchainCount),
          .pSetLayouts = layouts.data(),
  };
  aSurf->mDescriptorSets.resize(swapchainCount);
  CALL_VK(vkAllocateDescriptorSets(mDeviceInfo.device, &desAllocInfo, aSurf->mDescriptorSets.data()));

  for (size_t i = 0; i < swapchainCount; i++) {
    std::vector<VkWriteDescriptorSet> descriptorWrite;

    if (aSurf->mUniformBuffers.size()) {
      VkDescriptorBufferInfo bufferInfo {
        .buffer = aSurf->mUniformBuffers[i],
        .offset = 0,
        .range = aBufferSize,
      };

      descriptorWrite.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = aSurf->mDescriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &bufferInfo,
      });
    }

    if (aSurf->mTextures.size()) {
      // TODO: support multiple textures.
      VkDescriptorImageInfo imageInfo {
        // The image's view (images are never directly accessed by the shader,
        // but rather through views defining subresources)
        .imageView   = aSurf->mTextures[0].view,
        // The sampler (Telling the pipeline how to sample the texture,
        // including repeat, border, etc.)
        .sampler     = aSurf->mTextures[0].sampler,
        // The current layout of the image (Note: Should always fit
        // the actual use, e.g. shader read)
        .imageLayout = aSurf->mTextures[0].imageLayout,
      };

      descriptorWrite.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = aSurf->mDescriptorSets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &imageInfo,
      });
    }

    vkUpdateDescriptorSets(mDeviceInfo.device, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
  }
}

void VulkanRenderer::CreateCommandBuffer() {
  // Record a command buffer that just clear the screen
  // 1 command buffer draw in 1 framebuffer
  // In our case we need 2 command as we have 2 framebuffer
  mRenderInfo.cmdBufferLen = mSwapchain.swapchainLength;
  mRenderInfo.cmdBuffer = new VkCommandBuffer[mSwapchain.swapchainLength];
  VkCommandBufferAllocateInfo cmdBufferCreateInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = nullptr,
          .commandPool = mRenderInfo.cmdPool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = mRenderInfo.cmdBufferLen
  };
  CALL_VK(vkAllocateCommandBuffers(mDeviceInfo.device, &cmdBufferCreateInfo,
                                   mRenderInfo.cmdBuffer));
}

void VulkanRenderer::ConstructRenderPass() {
  CreateCommandBuffer();

  const int size = mSwapchain.swapchainLength;
  for (int bufferIndex = 0; bufferIndex < size; bufferIndex++) {
    // We start by creating and declare the "beginning" our command buffer
    VkCommandBufferBeginInfo cmdBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
    };
    CALL_VK(vkBeginCommandBuffer(mRenderInfo.cmdBuffer[bufferIndex],
                                 &cmdBufferBeginInfo));
    // transition the display image to color attachment layout
    SetImageLayout(mRenderInfo.cmdBuffer[bufferIndex],
                   mSwapchain.displayImages[bufferIndex],
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Now we start a renderpass. Any draw command has to be recorded in a
    // renderpass
    VkClearValue clearVals{
            .color.float32[0] = 0.1f,
            .color.float32[1] = 0.1f,
            .color.float32[2] = 0.2f,
            .color.float32[3] = 1.0f,
    };

    VkRenderPassBeginInfo renderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = mRenderInfo.renderPass,
            .framebuffer = mSwapchain.framebuffers[bufferIndex],
            .renderArea = {
                    .offset = {
                      .x = 0, .y = 0,
                    },
                    .extent = mSwapchain.displaySize
            },
            .clearValueCount = 1,
            .pClearValues = &clearVals
    };
    vkCmdBeginRenderPass(mRenderInfo.cmdBuffer[bufferIndex], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    for (const auto& surf : mSurfaces) {
      // Bind what is necessary to the command buffer
      vkCmdBindPipeline(mRenderInfo.cmdBuffer[bufferIndex],
                        VK_PIPELINE_BIND_POINT_GRAPHICS, surf->mGfxPipeline.pipeline);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(mRenderInfo.cmdBuffer[bufferIndex], 0, 1,
                             &surf->mBuffer.vertexBuf, &offset);

      if (surf->mBuffer.indexBuf) {
        vkCmdBindIndexBuffer(mRenderInfo.cmdBuffer[bufferIndex],
                             surf->mBuffer.indexBuf, 0, VK_INDEX_TYPE_UINT16);
      }

      if (surf->mDescriptorSets.size()) {
        vkCmdBindDescriptorSets(mRenderInfo.cmdBuffer[bufferIndex],
                                VK_PIPELINE_BIND_POINT_GRAPHICS, surf->mGfxPipeline.layout,
                                0, 1, &surf->mDescriptorSets[bufferIndex], 0, nullptr);
      }

      if (surf->mBuffer.indexBuf) {
        // Draw Triangle with indexed
        // commandBuffer, indexCount, instanceCount, firstVertex, vertexOffset, firstInstance
        vkCmdDrawIndexed(mRenderInfo.cmdBuffer[bufferIndex],
                         static_cast<uint32_t>(surf->mIndexData.size()), 1, 0, 0, 0);
      } else {
        // Draw Triangle
        // commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance
        vkCmdDraw(mRenderInfo.cmdBuffer[bufferIndex],
                  surf->mVertexCount, surf->mInstanceCount, surf->mFirstVertex, surf->mFirstInstance);
      }
    }

    vkCmdEndRenderPass(mRenderInfo.cmdBuffer[bufferIndex]);
    // transition back to swapchain image to PRESENT_SRC_KHR
    SetImageLayout(mRenderInfo.cmdBuffer[bufferIndex],
                   mSwapchain.displayImages[bufferIndex],
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    CALL_VK(vkEndCommandBuffer(mRenderInfo.cmdBuffer[bufferIndex]));
  }

  CreateSyncObjects();
}

//VkCommandBuffer VulkanRenderer::CreateCommandBuffer(VkCommandBufferLevel level, bool begin) {
//  assert(mRenderInfo.cmdPool && "No command pool exists in the device");
//
//  VkCommandBufferAllocateInfo cmdBufAllocateInfo {
//    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//    .commandPool = mRenderInfo.cmdPool,
//    .level = level,
//    .commandBufferCount = 1,
//  };
//
//  VkCommandBuffer commandBuffer;
//  CALL_VK(vkAllocateCommandBuffers(mDeviceInfo.device, &cmdBufAllocateInfo, &commandBuffer));
//
//  // If requested, also start recording for the new command buffer
//  if (begin)
//  {
//    VkCommandBufferBeginInfo commandBufferInfo {
//      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
//    };
//    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    CALL_VK(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));
//  }
//  return commandBuffer;
//}

//void VulkanRenderer::FlushCommandBuffer(VkCommandBuffer aCommandBuffer, VkQueue aQueue,
//                                        bool free, VkSemaphore aSignalSemaphore) {
//  if (aCommandBuffer == VK_NULL_HANDLE) {
//    return;
//  }
//
//  CALL_VK(vkEndCommandBuffer(aCommandBuffer));
//  VkSubmitInfo submitInfo {
//    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//    .commandBufferCount = 1,
//    .pCommandBuffers = &aCommandBuffer
//  };
//
//  if (aSignalSemaphore) {
//    submitInfo.pSignalSemaphores = &aSignalSemaphore;
//    submitInfo.signalSemaphoreCount = 1;
//  }
//
//  VkFenceCreateInfo fenceInfo {
//    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//    .flags = 0,
//  };
//
//  VkFence fence;
//  CALL_VK(vkCreateFence(mDeviceInfo.device, &fenceInfo, nullptr, &fence));
//
//  // Submit to the queue
//  CALL_VK(vkQueueSubmit(aQueue, 1, &submitInfo, fence));
//  // Wait for the fence to signal that command buffer has finished executing
//  const long long DEFAULT_FENCE_TIMEOUT = 100000000000;        // Default fence timeout in nanoseconds
//  CALL_VK(vkWaitForFences(mDeviceInfo.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
//  vkDestroyFence(mDeviceInfo.device, fence, nullptr);
//  // TODO: using vkQueueWaitIdle(graphicsQueue); to replace fence?
//
//  if (aCommandBuffer && free) {
//    vkFreeCommandBuffers(mDeviceInfo.device, mRenderInfo.cmdPool, 1, &aCommandBuffer);
//  }
//}

bool VulkanRenderer::CreateImage(const char* aFilePath, RenderSurface::VulkanTexture& aTexture, bool& aUseStaging) {
  std::string fileName(aFilePath);
  const int index = fileName.rfind('.');
  if (index == std::string::npos ||
      fileName.substr(index) != ".ktx") {
    LOG_E(gAppName.data(), "Load texture %s failed, we only support *.ktx format.", aFilePath);
    return false;
  }

  auto externalFilepath = Platform::GetExternalDirPath() + aFilePath;

  ktxTexture* ktxTexture;
  if (ktxTexture_CreateFromNamedFile(externalFilepath.c_str(),
                                     KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture) != KTX_SUCCESS ||
      !ktxTexture) {
    LOG_E(gAppName.data(), "Couldn't load %s image.", externalFilepath.c_str());
    return false;
  }

  // upload it to a Vulkan image object
  const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  aTexture.width = ktxTexture->baseWidth;
  aTexture.height = ktxTexture->baseHeight;
  aTexture.mipLevels = ktxTexture->numLevels;
  aTexture.format = format;

  // using stage buffer to copy the texture data to a device
  // local optimal memory.
  VkBool32 useStaging = true;
  bool forceLinearTiling = false;
  if (forceLinearTiling) {
    // Don't use linear if format is not supported for (linear) shader sampling
    // Get device properites for the requested texture format
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(mDeviceInfo.gpuDevice, format, &formatProperties);
    useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  }

  VkMemoryAllocateInfo memoryAllocateInfo {
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
  };
  VkMemoryRequirements memReq  = {};
  ktx_uint8_t* ktxImageData   = ktxTexture_GetData(ktxTexture);
  ktx_size_t   ktxTextureSize = ktxTexture_GetSize(ktxTexture);
  aUseStaging = useStaging;

  if (useStaging) {
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkBufferCreateInfo bufferCreateInfo {
            .size = ktxTextureSize,
            // This buffer is used as a transfer source for the buffer copy
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    CALL_VK(vkCreateBuffer(mDeviceInfo.device, &bufferCreateInfo, nullptr, &stagingBuffer));
    vkGetBufferMemoryRequirements(mDeviceInfo.device, stagingBuffer, &memReq);
    memoryAllocateInfo.allocationSize = memReq.size;
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &memoryAllocateInfo.memoryTypeIndex);
    CALL_VK(vkAllocateMemory(mDeviceInfo.device, &memoryAllocateInfo, nullptr, &stagingMemory));
    CALL_VK(vkBindBufferMemory(mDeviceInfo.device, stagingBuffer, stagingMemory, 0));

    uint8_t* data;
    CALL_VK(vkMapMemory(mDeviceInfo.device, stagingMemory, 0, memReq.size, 0, (void **)&data));
    memcpy(data, ktxImageData, ktxTextureSize);
    vkUnmapMemory(mDeviceInfo.device, stagingMemory);

    // Create optimal tiled target image on the device
    VkImageCreateInfo imageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .mipLevels = aTexture.mipLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .extent = {aTexture.width, aTexture.height, 1},
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    };
    CALL_VK(vkCreateImage(mDeviceInfo.device, &imageCreateInfo, nullptr, &aTexture.image));

    vkGetImageMemoryRequirements(mDeviceInfo.device, aTexture.image, &memReq);
    memoryAllocateInfo.allocationSize = memReq.size;
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         &memoryAllocateInfo.memoryTypeIndex);
    CALL_VK(vkAllocateMemory(mDeviceInfo.device, &memoryAllocateInfo, nullptr, &aTexture.deviceMemory));
    CALL_VK(vkBindImageMemory(mDeviceInfo.device, aTexture.image, aTexture.deviceMemory, 0));

    VkCommandBuffer copyCommand = BeginSingleTimeCommands();//CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    // Image memory barriers for the texture image
    // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
    VkImageSubresourceRange subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = aTexture.mipLevels,
            .layerCount = 1,
    };

    // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
    VkImageMemoryBarrier imageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = aTexture.image,
            .subresourceRange = subresourceRange,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    };

    // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
    // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
    // Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
    vkCmdPipelineBarrier(
            copyCommand,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    for (int i = 0; i < aTexture.mipLevels; i++) {
      ktx_size_t        offset;
      if (ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset) != KTX_SUCCESS) {
        LOG_E(gAppName.data(), "%s: Create mipmap level failed,", aFilePath);
        continue;
      }
      VkBufferImageCopy bufferCopyRegion               = {};
      bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      bufferCopyRegion.imageSubresource.mipLevel       = i;
      bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
      bufferCopyRegion.imageSubresource.layerCount     = 1;
      bufferCopyRegion.imageExtent.width               = ktxTexture->baseWidth >> i;
      bufferCopyRegion.imageExtent.height              = ktxTexture->baseHeight >> i;
      bufferCopyRegion.imageExtent.depth               = 1;
      bufferCopyRegion.bufferOffset                    = offset;
      bufferCopyRegions.push_back(bufferCopyRegion);
    }

    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(
            copyCommand,
            stagingBuffer,
            aTexture.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data());

    // Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
    // Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
    // Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
    vkCmdPipelineBarrier(
            copyCommand,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

    // Store current layout for later reuse
    aTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //FlushCommandBuffer(copyCommand, mDeviceInfo.graphicsQueue, true);
    EndSingleTimeCommands(copyCommand);

    // Clean up staging resources
    vkFreeMemory(mDeviceInfo.device, stagingMemory, nullptr);
    vkDestroyBuffer(mDeviceInfo.device, stagingBuffer, nullptr);
  } else {
    // Copy data to a linear tiled image
    VkImage        mappableImage;
    VkDeviceMemory mappableMemory;

    // Load mip map level 0 to linear tiling image
    VkImageCreateInfo imageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
            .extent = {aTexture.width, aTexture.height, 1},
    };

    CALL_VK(vkCreateImage(mDeviceInfo.device, &imageCreateInfo, nullptr, &mappableImage));

    // Get memory requirements for this image like size and alignment
    vkGetImageMemoryRequirements(mDeviceInfo.device, mappableImage, &memReq);
    // Set memory allocation size to required memory size
    memoryAllocateInfo.allocationSize = memReq.size;
    // Get memory type that can be mapped to host memory
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &memoryAllocateInfo.memoryTypeIndex);
    CALL_VK(vkAllocateMemory(mDeviceInfo.device, &memoryAllocateInfo, nullptr, &mappableMemory));
    CALL_VK(vkBindImageMemory(mDeviceInfo.device, mappableImage, mappableMemory, 0));

    // Map image memory
    void* data;
    ktx_size_t ktxImageSize = ktxTexture_GetImageSize(ktxTexture, 0);
    CALL_VK(vkMapMemory(mDeviceInfo.device, mappableMemory, 0, memReq.size, 0, &data));
    // Copy image data of the first mip level into memory
    memcpy(data, ktxImageData, ktxImageSize);
    vkUnmapMemory(mDeviceInfo.device, mappableMemory);

    // Linear tiled images don't need to be staged and can be directly used as textures
    aTexture.image = mappableImage;
    aTexture.deviceMemory = mappableMemory;
    aTexture.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Setup image memory barrier transfer image to shader read layout
    VkCommandBuffer copyCommand = BeginSingleTimeCommands(); //CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    // The sub resource range describes the regions of the image we will be transition
    VkImageSubresourceRange subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .layerCount = 1,
    };

    // Transition the texture image layout to shader read, so it can be sampled from
    VkImageMemoryBarrier imageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = aTexture.image,
            .subresourceRange = subresourceRange,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
    // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
    // Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
    vkCmdPipelineBarrier(
            copyCommand,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

    //FlushCommandBuffer(copyCommand, mDeviceInfo.graphicsQueue, true);
    EndSingleTimeCommands(copyCommand);
  }

  return true;
}

bool VulkanRenderer::CreateTextureFromFile(const char* aFilePath, std::shared_ptr<RenderSurface> aSurf) {
  bool useStaging = true;
  RenderSurface::VulkanTexture texture;
  CreateImage(aFilePath, texture, useStaging);

  // Create a texture sampler
  VkSamplerCreateInfo samplerInfo {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .maxAnisotropy = 1.0f,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .mipLodBias = 0.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_NEVER,
    .minLod = 0.0f,
    .maxLod = (useStaging) ? (float) texture.mipLevels : 0.0f,
  };

  if (mDeviceInfo.gpuDeviceFeatures.samplerAnisotropy) {
    samplerInfo.maxAnisotropy = mDeviceInfo.gpuDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.anisotropyEnable = VK_TRUE;
  } else {
    samplerInfo.maxAnisotropy = 1.0;
    samplerInfo.anisotropyEnable = VK_FALSE;
  }

  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  CALL_VK(vkCreateSampler(mDeviceInfo.device, &samplerInfo, nullptr, &texture.sampler));

  VkImageViewCreateInfo view {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = texture.format,
    .components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
    .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel   = 0,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount     = 1,
    // Linear tiling usually won't support mip maps
    // Only set mip map count if optimal tiling is used
    .subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1,
    // The view will be based on the texture's image
    .image = texture.image,
  };
  CALL_VK(vkCreateImageView(mDeviceInfo.device, &view, nullptr, &texture.view));
  aSurf->mTextures.push_back(texture);

  return true;
}

bool VulkanRenderer::IsReady() {
  return mInitialized;
}

void VulkanRenderer::DestroyShaderModule(VkShaderModule aShader) {
  vkDestroyShaderModule(mDeviceInfo.device, aShader, nullptr);
}

void VulkanRenderer::DeleteSwapChain() {
  for (int i = 0; i < mSwapchain.displayImages.size(); i++) {
    vkDestroyFramebuffer(mDeviceInfo.device, mSwapchain.framebuffers[i], nullptr);
    vkDestroyImageView(mDeviceInfo.device, mSwapchain.displayViews[i], nullptr);
    vkDestroyImage(mDeviceInfo.device, mSwapchain.displayImages[i], nullptr);

    for (const auto& surf : mSurfaces) {
      vkDestroyBuffer(mDeviceInfo.device, surf->mUniformBuffers[i], nullptr);
      vkFreeMemory(mDeviceInfo.device, surf->mUniformBuffersMemory[i], nullptr);
    }
  }
  vkDestroySwapchainKHR(mDeviceInfo.device, mSwapchain.swapchain, nullptr);
}

void VulkanRenderer::DeleteGraphicsPipeline() {
  for (auto& surf : mSurfaces) {
    if (surf->mGfxPipeline.pipeline == VK_NULL_HANDLE) {
      continue;
    }
    vkDestroyPipeline(mDeviceInfo.device, surf->mGfxPipeline.pipeline, nullptr);
    vkDestroyPipelineCache(mDeviceInfo.device, surf->mGfxPipeline.cache, nullptr);
    vkDestroyPipelineLayout(mDeviceInfo.device, surf->mGfxPipeline.layout, nullptr);
  }
}

void VulkanRenderer::DeleteTextures() {
  // delete from surface
  for (const auto& surf : mSurfaces) {
    for (const auto& tex : surf->mTextures) {
      vkDestroyImage(mDeviceInfo.device, tex.image, nullptr);
      vkDestroyImageView(mDeviceInfo.device, tex.view, nullptr);
      vkDestroySampler(mDeviceInfo.device, tex.sampler, nullptr);
      vkFreeMemory(mDeviceInfo.device, tex.deviceMemory, nullptr);
    }
    surf->mTextures.clear();
  }
}

void VulkanRenderer::DeleteBuffers() {
  for (const auto& surf : mSurfaces) {
    if (surf->mBuffer.vertexBuf) {
      vkDestroyBuffer(mDeviceInfo.device, surf->mBuffer.vertexBuf, nullptr);
      vkFreeMemory(mDeviceInfo.device, surf->mBuffer.vertexBufMemory, nullptr);
    }
    if (surf->mBuffer.indexBuf) {
      vkDestroyBuffer(mDeviceInfo.device, surf->mBuffer.indexBuf, nullptr);
      vkFreeMemory(mDeviceInfo.device, surf->mBuffer.indexBufMemory, nullptr);
    }
  }
}

void VulkanRenderer::DeleteDescriptors() {
  for (const auto& surf : mSurfaces) {
    if (surf->mDescriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(mDeviceInfo.device, surf->mDescriptorSetLayout, nullptr);
    }
    if (surf->mDescriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(mDeviceInfo.device, surf->mDescriptorPool, nullptr);
    }
  }
}

void VulkanRenderer::Terminate() {
  vkFreeCommandBuffers(mDeviceInfo.device, mRenderInfo.cmdPool, mRenderInfo.cmdBufferLen,
                       mRenderInfo.cmdBuffer);
  delete[] mRenderInfo.cmdBuffer;

  vkDestroyCommandPool(mDeviceInfo.device, mRenderInfo.cmdPool, nullptr);
  vkDestroyRenderPass(mDeviceInfo.device, mRenderInfo.renderPass, nullptr);
  DeleteSwapChain();
  DeleteGraphicsPipeline();
  DeleteBuffers();
  DeleteTextures();
  DeleteDescriptors();

  if (enableValidationLayers && mDeviceInfo.debugReportCallback != VK_NULL_HANDLE) {
    vkDestroyDebugReportCallbackEXT(mDeviceInfo.instance, mDeviceInfo.debugReportCallback, nullptr);
    mDeviceInfo.debugReportCallback = VK_NULL_HANDLE;
  }
  vkDestroySurfaceKHR(mDeviceInfo.instance, mDeviceInfo.surface, nullptr);
  vkDestroyDevice(mDeviceInfo.device, nullptr);
  vkDestroyInstance(mDeviceInfo.instance, nullptr);

  mInitialized = false;
}

void VulkanRenderer::RenderFrame() {
  uint32_t nextIndex;
  // Get the framebuffer index we should draw in
  CALL_VK(vkAcquireNextImageKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                UINT64_MAX,  mRenderInfo.semaphore, VK_NULL_HANDLE,
                                &nextIndex));
  UpdateUniformBuffer(nextIndex);

  // TODO: add VkSemaphore when vulkan is running in multi-thread.
  CALL_VK(vkResetFences(mDeviceInfo.device, 1, &mRenderInfo.fence));

  VkPipelineStageFlags waitStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .pNext = nullptr,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &mRenderInfo.semaphore,
          .pWaitDstStageMask = &waitStageMask,
          .commandBufferCount = 1,
          .pCommandBuffers = &mRenderInfo.cmdBuffer[nextIndex],
          .signalSemaphoreCount = 0,
          .pSignalSemaphores = nullptr};
  CALL_VK(vkQueueSubmit(mDeviceInfo.presentqueue, 1, &submit_info, mRenderInfo.fence));
  CALL_VK(vkWaitForFences(mDeviceInfo.device, 1, &mRenderInfo.fence, VK_TRUE, 100000000));

  VkResult result;
  VkPresentInfoKHR presentInfo{
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = nullptr,
          .swapchainCount = 1,
          .pSwapchains = &mSwapchain.swapchain,
          .pImageIndices = &nextIndex,
          .waitSemaphoreCount = 0,
          .pWaitSemaphores = nullptr,
          .pResults = &result,
  };
  vkQueuePresentKHR(mDeviceInfo.presentqueue, &presentInfo);
}

bool VulkanRenderer::AddSurface(std::shared_ptr<RenderSurface> aSurf) {
  mSurfaces.push_back(aSurf);
  return true;
}

