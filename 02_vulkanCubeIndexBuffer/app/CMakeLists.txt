cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${APP_GLUE_DIR})
add_library(app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)

# build vulkan app
set(SRC_JNI_DIR src/main/jni)
set(SRC_RENDERER_DIR ../../common/renderer)
set(WRAPPER_DIR ../../common/vulkan_wrapper)
set(UTILS_DIR ../../common/utils)

add_library(vktuts SHARED
        ${SRC_JNI_DIR}/AndroidMain.cpp
        ${SRC_JNI_DIR}/VulkanMain.cpp
        ${WRAPPER_DIR}/vulkan_wrapper.cpp
        ${SRC_RENDERER_DIR}/VulkanRenderer.cpp
        ${SRC_RENDERER_DIR}/Object.cpp
        ${SRC_RENDERER_DIR}/component/Material.cpp
        )

include_directories(${WRAPPER_DIR}
        ${UTILS_DIR}
        ${SRC_RENDERER_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror \
                     -DVK_USE_PLATFORM_ANDROID_KHR")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
target_link_libraries(vktuts app-glue log android)
