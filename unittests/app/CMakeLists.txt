cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${APP_GLUE_DIR})
add_library(app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)

set(SRC_JNI_DIR src/main/jni)
set(UTILS_DIR ../../common/utils)
set(THIRD_PARTY_DIR ../../third_party)
set(GTEST_DIR ../../third_party/googletest)
set(TEST_SRC_DIR src/test/cpp)

add_library(vkexamples SHARED
            ${SRC_JNI_DIR}/AndroidMain.cpp
            ${SRC_JNI_DIR}/VulkanMain.cpp
            ${SRC_JNI_DIR}/GTestRunnerJNI.cpp
            ${TEST_SRC_DIR}/Tests.cpp)

include_directories(${UTILS_DIR}
                    ${THIRD_PARTY_DIR}/gfx-math/include)

# Add third party libraries
add_subdirectory(${THIRD_PARTY_DIR} third_party)
add_subdirectory(${GTEST_DIR} gtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror \
                     -DVK_USE_PLATFORM_ANDROID_KHR")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(vkexamples app-glue log android gtest)
