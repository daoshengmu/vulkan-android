//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKANTESTS_VULKANMAIN_H
#define VULKANTESTS_VULKANMAIN_H

#include <android_native_app_glue.h>

bool InitVulkan(android_app *app);

void TerminateVulkan();

bool IsVulkanReady();

bool VulkanRenderFrame();

#endif //VULKANTESTS_VULKANMAIN_H

