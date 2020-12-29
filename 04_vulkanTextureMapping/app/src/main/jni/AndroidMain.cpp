//
// Created by Daosheng Mu on 8/8/20.
//

#include <android/log.h>
#include <android_native_app_glue.h>
#include "Utils.h"
#include "Platform.h"
#include "VulkanMain.h"

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_vulkanTextureMapping_MainActivity_initFilePath(JNIEnv *env,
                          jobject thiz, jstring externalDir) {
  const char* externalDirCStr = env->GetStringUTFChars(externalDir, 0);;
  Platform::SetExternalDirPath(externalDirCStr);
  env->ReleaseStringUTFChars(externalDir, externalDirCStr);
}

}

void CmdHandler(android_app* app, int32_t cmd) {
   switch (cmd) {
      case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        InitVulkan(app);
        break;
      case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        TerminateVulkan();
        break;
      default:
        __android_log_print(ANDROID_LOG_INFO, "Vulkan Tutorials",
                            "event not handled: %d", cmd);
    }
}

// typical Android NativeActivity entry function
void android_main(struct android_app* app) {
  app->onAppCmd = CmdHandler;

  // Polling events from the main loop
  int events;
  android_poll_source* source;

  // Main loop
  do {
    if (ALooper_pollAll(IsVulkanReady() ? 1 : 0, nullptr,
                        &events, (void**)&source) >= 0) {
      if (source != NULL) source->process(app, source);
    }

    // render if vulkan is ready
    if (IsVulkanReady()) {
      VulkanRenderFrame();
    }
  } while (app->destroyRequested == 0);
}
