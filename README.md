# vulkan-android
The purpose of this project is in order to provide step-by-step examples of learning Vulkan. The current examples work well on Android phones which are capable for Vulkan. They have been confirmed running well in AndroidStudio. (*Maybe I would consider making it support macOS platform soon.*)

## Support Platforms
- [x] **Android**: Android Studio 4.0, API 29, NDK 21, Gradle 6.1.1
- [ ] **macOS**
- [ ] **Windows**

## Setup Instructions
*Clone.*
```
git clone git@github.com:daoshengmu/vulkan-android.git
cd vulkan-android
```
*Fetch Git submodules.*
```
git submodule update --init --recursive
```
*Update `gfx-math` only.*

Due to we stick with `KTX-Software` v3.0, we can't easily just *git submodule update* for updating all submodules totally, we have to update the submodules respectively to avoid KTX-Software folder is updated.
```
git submodule update --remote third_party/gfx-math/
```

## Sync Files
Due to we use `fopen` to access textures from the storage instead of Android asset manager, we need to sync the texture files manually.
```
adb push --sync assets/ /storage/emulated/0/Android/data/com.example.{PROJECT_NAME}/files/
```

## Run Build and Tests
We are currently running automatic build in Circle-CI and partial automatic tests in Github actions because Circle-CI do not support Android emulator launching in their Linux VM.

**Run Build**: Go to a sub-folder, execute `./gradlew build`.

**Run Tests**: Go to *unittests/* folder, execute `./gradlew connectedCheck`.

![CI](https://github.com/daoshengmu/vulkan-android/workflows/CI/badge.svg)

[![CircleCI](https://circleci.com/gh/daoshengmu/vulkan-android.svg?style=shield)](https://circleci.com/gh/daoshengmu/vulkan-android)

## Examples
### Basic
- [x] Triangle: Using a basic shader and vertex buffer to present how to draw primitives in Vulkan.
- [x] Cube index buffer: Introduce how to use a index buffer combines with a vertex buffer to draw indexed primitives. 
- [x] Projection transform: Describe how to use a uniform buffer to send a model-view-projection matrix to a vertex shader, and show how to load Vulkan layer properties from Vulkan jni library.
- [x] Texture mapping: Giving an example of how to load and create a texture in Vulkan, Then, in fragement shaders, how we fetch the texels of a texture.

### Advance
- [ ] *glTF* model loader: Provide a *glTF* model loader and display *glTF* in Vulkan.
- [ ] Video playback with Vulkan.
- [ ] Physically-based rendering: PBR material support.

## Additional Resources
- Vulkan Tutorial, https://vulkan-tutorial.com
- googlesamples/android-vulkan-tutorials, https://github.com/googlesamples/android-vulkan-tutorials
- KhronosGroup/Vulkan-Samples, https://github.com/KhronosGroup/Vulkan-Samples