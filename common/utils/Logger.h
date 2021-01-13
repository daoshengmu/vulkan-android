//
// Created by Daosheng Mu on 8/8/20.
//

#ifndef VULKANANDROID_COMMONUTILS_LOGGER_H
#define VULKANANDROID_COMMONUTILS_LOGGER_H

#include <android/log.h>

#define LOG_I(TAG, ...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOG_W(TAG, ...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOG_E(TAG, ...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))


#endif //VULKANANDROID_COMMONUTILS_LOGGER_H
