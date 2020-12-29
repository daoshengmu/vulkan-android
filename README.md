# vulkan-android
The purpose of this project is in order to provide step-by-step examples of learning Vulkan. The current examples work well on Android phones which are capable for Vulkan. They have been confirmed running well in AndroidStudio. (Maybe I would make they also be able to run on Mac OS platform in the near future.)

## Setup instructions
*Clone.*
```
git clone git@github.com:daoshengmu/vulkan-android.git
cd vulkan-android
```
*Fetch Git submodules.*
```
git submodule update --init --recursive
```

## Sync files
Due to we use `fopen` to access textures from the storage instead of Android asset manager, we need to sync the texture files manually.
```
adb push assets/ /storage/emulated/0/Android/data/com.example.{PROJECT_NAME}/files/assets
```

## Examples
### Basic
- [x] 1. Triangle: Using a basic shader and vertex buffer to present how to draw primitives in Vulkan.
- [x] 2. Cube index buffer: Introduce how to use a index buffer combines with a vertex buffer to draw indexed primitives. 
- [x] 3. Projection transform: Describe how to use a uniform buffer to send a model-view-projection matrix to a vertex shader, and show how to load Vulkan layer properties from Vulkan jni library.
- [x] 4. Texture mapping: Giving an example of how to load and create a texture in Vulkan, Then, in fragement shaders, how we fetch the texels of a texture.

### Advance
- [ ] 5. gltf model loader: Provide a gltf model loader and display gltf in Vulkan.
- [ ] 6. Physically-based rendering: PBR material support.
- [ ] 7. Video playback with Vulkan.

## Useful Resource
- Vulkan Tutorial, https://vulkan-tutorial.com
- googlesamples/android-vulkan-tutorials, https://github.com/googlesamples/android-vulkan-tutorials
- KhronosGroup/Vulkan-Samples, https://github.com/KhronosGroup/Vulkan-Samples