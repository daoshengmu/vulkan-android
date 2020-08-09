//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanRenderer.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <vector>
#include "vulkan_wrapper.h"
#include "utils.h"

static std::string gAppName;

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, gAppName.c_str(),          \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

void VulkanRenderer::CreateVulkanDevice(ANativeWindow* platformWindow,
                                        VkApplicationInfo* appInfo) {
  LOG_I(gAppName.c_str(), "CreateVulkanDevice");
  std::vector<const char*> instance_extensions;
  std::vector<const char*> device_extensions;

  instance_extensions.push_back("VK_KHR_surface");
  instance_extensions.push_back("VK_KHR_android_surface");

  device_extensions.push_back("VK_KHR_swapchain");

  // **********************************************************
  // Create the Vulkan instance
  VkInstanceCreateInfo instanceCreateInfo{
          .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
          .pNext = nullptr,
          .pApplicationInfo = appInfo,
          .enabledExtensionCount =
          static_cast<uint32_t>(instance_extensions.size()),
          .ppEnabledExtensionNames = instance_extensions.data(),
          .enabledLayerCount = 0,
          .ppEnabledLayerNames = nullptr,
  };
  CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &mDeviceInfo.instance));
  VkAndroidSurfaceCreateInfoKHR createInfo{
          .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
          .pNext = nullptr,
          .flags = 0,
          .window = platformWindow};

  CALL_VK(vkCreateAndroidSurfaceKHR(mDeviceInfo.instance, &createInfo, nullptr,
                                    &mDeviceInfo.surface));

  // Find one GPU to use:
  // On Android, every GPU device is equal -- supporting
  // graphics/compute/present
  // for this sample, we use the very first GPU device found on the system
  uint32_t gpuCount = 0;
  CALL_VK(vkEnumeratePhysicalDevices(mDeviceInfo.instance, &gpuCount, nullptr));
  assert(gpuCount);
  VkPhysicalDevice tmpGpus[gpuCount];
  CALL_VK(vkEnumeratePhysicalDevices(mDeviceInfo.instance, &gpuCount, tmpGpus));
  mDeviceInfo.gpuDevice = tmpGpus[0];  // Pick up the first GPU Device

  // Find a GFX queue family
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(mDeviceInfo.gpuDevice, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(mDeviceInfo.gpuDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());

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

  // Create a logical device (vulkan device)
  float priorities[] = {
          1.0f,
  };
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
  vkGetDeviceQueue(mDeviceInfo.device, mDeviceInfo.queueFamilyIndex, 0, &mDeviceInfo.queue);
}

void VulkanRenderer::CreateSwapChain() {
  LOG_I(gAppName.c_str(), "CreateSwapChain");
  memset(&mSwapchain, 0, sizeof(mSwapchain));

  // **********************************************************
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

  uint32_t chosenFormat;
  for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
    if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
  }
  assert(chosenFormat < formatCount);

  mSwapchain.displaySize = surfaceCapabilities.currentExtent;
  mSwapchain.displayFormat = formats[chosenFormat].format;

  // **********************************************************
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

bool VulkanRenderer::Init(android_app* app, std::string aAppName) {
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

  // Create 2 frame buffers.
  CreateFrameBuffers(mRenderInfo.renderPass);

  CreateBuffers();  // create vertex buffers

  // Create graphics pipeline
  CreateGraphicsPipeline();

  // -----------------------------------------------
  // Create a pool of command buffers to allocate command buffer from
  VkCommandPoolCreateInfo cmdPoolCreateInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .pNext = nullptr,
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .queueFamilyIndex = mDeviceInfo.queueFamilyIndex,
  };
  CALL_VK(vkCreateCommandPool(mDeviceInfo.device, &cmdPoolCreateInfo, nullptr,
                              &mRenderInfo.cmdPool));

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

  for (int bufferIndex = 0; bufferIndex < mSwapchain.swapchainLength;
       bufferIndex++) {
    // We start by creating and declare the "beginning" our command buffer
    VkCommandBufferBeginInfo cmdBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
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
            .color.float32[0] = 0.0f,
            .color.float32[1] = 0.34f,
            .color.float32[2] = 0.90f,
            .color.float32[3] = 1.0f,
    };

    VkRenderPassBeginInfo renderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = mRenderInfo.renderPass,
            .framebuffer = mSwapchain.framebuffers[bufferIndex],
            .renderArea = {.offset =
                    {
                            .x = 0, .y = 0,
                    },
                    .extent = mSwapchain.displaySize},
            .clearValueCount = 1,
            .pClearValues = &clearVals};
    vkCmdBeginRenderPass(mRenderInfo.cmdBuffer[bufferIndex], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    // Bind what is necessary to the command buffer
    vkCmdBindPipeline(mRenderInfo.cmdBuffer[bufferIndex],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, mGfxPipeline.pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(mRenderInfo.cmdBuffer[bufferIndex], 0, 1,
                           &mBuffer.vertexBuf, &offset);

    // Draw Triangle
    vkCmdDraw(mRenderInfo.cmdBuffer[bufferIndex], 3, 1, 0, 0);

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

  // We need to create a fence to be able, in the main loop, to wait for our
  // draw command(s) to finish before swapping the framebuffers
  VkFenceCreateInfo fenceCreateInfo{
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
  };
  CALL_VK(
          vkCreateFence(mDeviceInfo.device, &fenceCreateInfo, nullptr, &mRenderInfo.fence));

  // We need to create a semaphore to be able to wait, in the main loop, for our
  // framebuffer to be available for us before drawing.
  VkSemaphoreCreateInfo semaphoreCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
  };
  CALL_VK(vkCreateSemaphore(mDeviceInfo.device, &semaphoreCreateInfo, nullptr,
                            &mRenderInfo.semaphore));

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

VkResult VulkanRenderer::loadShaderFromFile(const char* filePath, VkShaderModule* shaderOut,
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
  VkResult result = vkCreateShaderModule(
          mDeviceInfo.device, &shaderModuleCreateInfo, nullptr, shaderOut);
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
          .subresourceRange =
                  {
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


void VulkanRenderer::CreateFrameBuffers(VkRenderPass& renderPass,
                                        VkImageView depthView) {
  // query display attachment to swapchain
  uint32_t SwapchainImagesCount = 0;
  CALL_VK(vkGetSwapchainImagesKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                  &SwapchainImagesCount, nullptr));
  mSwapchain.displayImages.resize(SwapchainImagesCount);
  CALL_VK(vkGetSwapchainImagesKHR(mDeviceInfo.device, mSwapchain.swapchain,
                                  &SwapchainImagesCount,
                                  mSwapchain.displayImages.data()));

  // create image view for each swapchain image
  mSwapchain.displayViews.resize(SwapchainImagesCount);
  for (uint32_t i = 0; i < SwapchainImagesCount; i++) {
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
      mSwapchain.displayViews[i], depthView,
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

void VulkanRenderer::CreateBuffers() {
  // -----------------------------------------------
  // Create the triangle vertex buffer

  // Vertex positions
  const float vertexData[] = {
          -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  };

  // Create a vertex buffer
  VkBufferCreateInfo createBufferInfo{
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .pNext = nullptr,
          .size = sizeof(vertexData),
          .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          .flags = 0,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .pQueueFamilyIndices = &mDeviceInfo.queueFamilyIndex,
          .queueFamilyIndexCount = 1,
  };

  CALL_VK(vkCreateBuffer(mDeviceInfo.device, &createBufferInfo, nullptr,
                         &mBuffer.vertexBuf));

  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(mDeviceInfo.device, mBuffer.vertexBuf, &memReq);

  VkMemoryAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          .pNext = nullptr,
          .allocationSize = memReq.size,
          .memoryTypeIndex = 0,  // Memory type assigned in the next step
  };

  // Assign the proper memory type for that buffer
  MapMemoryTypeToIndex(memReq.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &allocInfo.memoryTypeIndex);

  // Allocate memory for the buffer
  VkDeviceMemory deviceMemory;
  CALL_VK(vkAllocateMemory(mDeviceInfo.device, &allocInfo, nullptr, &deviceMemory));

  void* data;
  CALL_VK(vkMapMemory(mDeviceInfo.device, deviceMemory, 0, allocInfo.allocationSize,
                      0, &data));
  memcpy(data, vertexData, sizeof(vertexData));
  vkUnmapMemory(mDeviceInfo.device, deviceMemory);

  CALL_VK(
          vkBindBufferMemory(mDeviceInfo.device, mBuffer.vertexBuf, deviceMemory, 0));
  return;
}

VkResult VulkanRenderer::CreateGraphicsPipeline() {
  memset(&mGfxPipeline, 0, sizeof(mGfxPipeline));
  // Create pipeline layout (empty)
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .pNext = nullptr,
          .setLayoutCount = 0,
          .pSetLayouts = nullptr,
          .pushConstantRangeCount = 0,
          .pPushConstantRanges = nullptr,
  };
  CALL_VK(vkCreatePipelineLayout(mDeviceInfo.device, &pipelineLayoutCreateInfo,
                                 nullptr, &mGfxPipeline.layout));

  // No dynamic state in that tutorial
  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
          .pNext = nullptr,
          .dynamicStateCount = 0,
          .pDynamicStates = nullptr};

  VkShaderModule vertexShader, fragmentShader;
  loadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
  loadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);

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
          }};

  VkViewport viewports{
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
          .x = 0,
          .y = 0,
          .width = (float)mSwapchain.displaySize.width,
          .height = (float)mSwapchain.displaySize.height,
  };

  VkRect2D scissor = {.extent = mSwapchain.displaySize,
          .offset = {
                  .x = 0, .y = 0,
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

  // Specify color blend state
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
          .cullMode = VK_CULL_MODE_NONE,
          .frontFace = VK_FRONT_FACE_CLOCKWISE,
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
          .stride = 3 * sizeof(float),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  VkVertexInputAttributeDescription vertex_input_attributes[1]{{
                                                                       .binding = 0,
                                                                       .location = 0,
                                                                       .format = VK_FORMAT_R32G32B32_SFLOAT,
                                                                       .offset = 0,
                                                               }};
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
          .pNext = nullptr,
          .vertexBindingDescriptionCount = 1,
          .pVertexBindingDescriptions = &vertex_input_bindings,
          .vertexAttributeDescriptionCount = 1,
          .pVertexAttributeDescriptions = vertex_input_attributes,
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
                                      &mGfxPipeline.cache));

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
          .layout = mGfxPipeline.layout,
          .renderPass = mRenderInfo.renderPass,
          .subpass = 0,
          .basePipelineHandle = VK_NULL_HANDLE,
          .basePipelineIndex = 0,
  };

  VkResult pipelineResult = vkCreateGraphicsPipelines(
          mDeviceInfo.device, mGfxPipeline.cache, 1, &pipelineCreateInfo, nullptr,
          &mGfxPipeline.pipeline);

  // We don't need the shaders anymore, we can release their memory
  vkDestroyShaderModule(mDeviceInfo.device, vertexShader, nullptr);
  vkDestroyShaderModule(mDeviceInfo.device, fragmentShader, nullptr);

  return pipelineResult;
}

bool VulkanRenderer::IsReady() {
  return mInitialized;
}

void VulkanRenderer::DeleteSwapChain() {
  for (int i = 0; i < mSwapchain.swapchainLength; i++) {
    vkDestroyFramebuffer(mDeviceInfo.device, mSwapchain.framebuffers[i], nullptr);
    vkDestroyImageView(mDeviceInfo.device, mSwapchain.displayViews[i], nullptr);
    vkDestroyImage(mDeviceInfo.device, mSwapchain.displayImages[i], nullptr);
  }
  vkDestroySwapchainKHR(mDeviceInfo.device, mSwapchain.swapchain, nullptr);
}

void VulkanRenderer::DeleteGraphicsPipeline() {
  if (mGfxPipeline.pipeline == VK_NULL_HANDLE) return;
  vkDestroyPipeline(mDeviceInfo.device, mGfxPipeline.pipeline, nullptr);
  vkDestroyPipelineCache(mDeviceInfo.device, mGfxPipeline.cache, nullptr);
  vkDestroyPipelineLayout(mDeviceInfo.device, mGfxPipeline.layout, nullptr);
}

void VulkanRenderer::DeleteBuffers() {
  vkDestroyBuffer(mDeviceInfo.device, mBuffer.vertexBuf, nullptr);
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
  CALL_VK(vkQueueSubmit(mDeviceInfo.queue, 1, &submit_info, mRenderInfo.fence));
  CALL_VK(
          vkWaitForFences(mDeviceInfo.device, 1, &mRenderInfo.fence, VK_TRUE, 100000000));

  LOG_I(gAppName.c_str(), "Rendering frames......");

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
  vkQueuePresentKHR(mDeviceInfo.queue, &presentInfo);
}
