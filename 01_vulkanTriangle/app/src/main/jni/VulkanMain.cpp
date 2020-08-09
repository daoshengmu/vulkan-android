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

// Android log function wrappers
static const char* kTAG = "01-Vulkan-Triangle";

//#define LOG_TT(...) \
//  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Tutorial ",               \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }


// Android Native App pointer...
android_app* androidAppCtx = nullptr;
VulkanRenderer gRenderer;

// Global Variables ...
struct VulkanDeviceInfo {
  bool initialized_;

  VkInstance instance_;
  VkPhysicalDevice gpuDevice_;
  VkDevice device_;
  uint32_t queueFamilyIndex_;

  VkSurfaceKHR surface_;
  VkQueue queue_;
};
VulkanDeviceInfo device;

// Create vulkan device
//void CreateVulkanDevice(ANativeWindow* platformWindow,
//                        VkApplicationInfo* appInfo) {
//  std::vector<const char*> instance_extensions;
//  std::vector<const char*> device_extensions;
//
//  instance_extensions.push_back("VK_KHR_surface");
//  instance_extensions.push_back("VK_KHR_android_surface");
//
//  device_extensions.push_back("VK_KHR_swapchain");
//
//  // **********************************************************
//  // Create the Vulkan instance
//  VkInstanceCreateInfo instanceCreateInfo{
//          .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//          .pNext = nullptr,
//          .pApplicationInfo = appInfo,
//          .enabledExtensionCount =
//          static_cast<uint32_t>(instance_extensions.size()),
//          .ppEnabledExtensionNames = instance_extensions.data(),
//          .enabledLayerCount = 0,
//          .ppEnabledLayerNames = nullptr,
//  };
//  CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &device.instance_));
//  VkAndroidSurfaceCreateInfoKHR createInfo{
//          .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
//          .pNext = nullptr,
//          .flags = 0,
//          .window = platformWindow};
//
//  CALL_VK(vkCreateAndroidSurfaceKHR(device.instance_, &createInfo, nullptr,
//                                    &device.surface_));
//  // Find one GPU to use:
//  // On Android, every GPU device is equal -- supporting
//  // graphics/compute/present
//  // for this sample, we use the very first GPU device found on the system
//  uint32_t gpuCount = 0;
//  CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, nullptr));
//  assert(gpuCount);
//  VkPhysicalDevice tmpGpus[gpuCount];
//  CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, tmpGpus));
//  device.gpuDevice_ = tmpGpus[0];  // Pick up the first GPU Device
//
//  // Find a GFX queue family
//  uint32_t queueFamilyCount;
//  vkGetPhysicalDeviceQueueFamilyProperties(device.gpuDevice_, &queueFamilyCount,
//                                           nullptr);
//  assert(queueFamilyCount);
//  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
//  vkGetPhysicalDeviceQueueFamilyProperties(device.gpuDevice_, &queueFamilyCount,
//                                           queueFamilyProperties.data());
//
//  uint32_t queueFamilyIndex;
//  for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
//       queueFamilyIndex++) {
//    if (queueFamilyProperties[queueFamilyIndex].queueFlags &
//        VK_QUEUE_GRAPHICS_BIT) {
//      break;
//    }
//  }
//  assert(queueFamilyIndex < queueFamilyCount);
//  device.queueFamilyIndex_ = queueFamilyIndex;
//
//  // Create a logical device (vulkan device)
//  float priorities[] = {
//          1.0f,
//  };
//  VkDeviceQueueCreateInfo queueCreateInfo{
//          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//          .pNext = nullptr,
//          .flags = 0,
//          .queueCount = 1,
//          .queueFamilyIndex = queueFamilyIndex,
//          .pQueuePriorities = priorities,
//  };
//
//  VkDeviceCreateInfo deviceCreateInfo{
//          .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//          .pNext = nullptr,
//          .queueCreateInfoCount = 1,
//          .pQueueCreateInfos = &queueCreateInfo,
//          .enabledLayerCount = 0,
//          .ppEnabledLayerNames = nullptr,
//          .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
//          .ppEnabledExtensionNames = device_extensions.data(),
//          .pEnabledFeatures = nullptr,
//  };
//
//  CALL_VK(vkCreateDevice(device.gpuDevice_, &deviceCreateInfo, nullptr,
//                         &device.device_));
//  vkGetDeviceQueue(device.device_, device.queueFamilyIndex_, 0, &device.queue_);
//}

bool InitVulkan(android_app* app) {
  androidAppCtx = app;

  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

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