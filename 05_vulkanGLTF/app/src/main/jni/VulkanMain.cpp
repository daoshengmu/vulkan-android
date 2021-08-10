//
// Created by Daosheng Mu on 8/8/20.
//

#include "VulkanMain.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <string>
#include <MathUtils.h>
#include <Platform.h>
#include <iostream>
#include <ktx.h>
#include <tiny_gltf.h>

#include "Logger.h"
#include "vulkan_wrapper.h"
#include "VulkanRenderer.h"
#include "Matrix4x4.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

using namespace tinygltf;
static const char* kTAG = "05-Vulkan-glTF";

VulkanRenderer gRenderer;
std::shared_ptr<RenderSurface> gSurf = std::make_shared<RenderSurface>();

void PrintNodes(const tinygltf::Scene &scene) {
  for (size_t i = 0; i < scene.nodes.size(); i++) {
    LOG_I(kTAG, "node.name : %d", scene.nodes[i]);
  }
}

size_t ComponentTypeByteSize(int type) {
  switch (type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      return sizeof(char);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      return sizeof(short);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    case TINYGLTF_COMPONENT_TYPE_INT:
      return sizeof(int);
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
      return sizeof(float);
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
      return sizeof(double);
    default:
      return 0;
  }
}

void SetupMeshState(const tinygltf::Model& model) {

  // Handle buffers:
  // vertex
  // index

  // vertex type format

  // For each buffer in bufferViews to get buffer data.
  // BufferView: it includes vertex and index data.
  // Index data usually owns scalar type, and vertex data is usually vector3.

  // A bufferView represents a subset of data in a buffer, defined by a byte offset
  // into the buffer specified in the byteOffset property and a total byte length specified
  // by the byteLength property of the buffer view.
  for (size_t i = 0; i < model.bufferViews.size(); i++) {
    const tinygltf::BufferView& bufferView = model.bufferViews[i];
    int componentType;
    int accessorType;
    if (bufferView.target == 0) {
      LOG_W(kTAG, "WARN: bufferView.target is zero.");
      continue;
    }

    int sparseAccessor = -1;
    for (size_t j = 0; j < model.accessors.size(); j++) {
      const auto& accessor = model.accessors[j];
      if (accessor.bufferView == i) {
        LOG_I(kTAG, "%zu is used by accessor", j);
        componentType = accessor.componentType;
        accessorType = accessor.type;

        if (accessor.sparse.isSparse) {
          LOG_W(kTAG, "WARN: this bufferView has at least one sparse accessor to \"\n"
                      "it. We are going to load the data as patched by this \"\n"
                      "sparse accessor, not the original data.");
          sparseAccessor = j;
          break;
        }
      }
    }

    // Using bufferView id to process buffers.
    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
    LOG_I(kTAG, "glTF buffer.size = %zu, byteOffset = %zu", buffer.data.size(),
          bufferView.byteOffset);

    if (sparseAccessor < 0) {
      // Store buffer data
      // No sparse data.
      // just create buffers
      if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {
        // Create index buffer
        if (componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
          LOG_E(kTAG, "Index data format is not supported.");
          continue;
        }

        gSurf->mIndexCount = bufferView.byteLength / 2;
        std::vector<uint16_t> indexData(gSurf->mIndexCount);
        memcpy(indexData.data(), buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);
        gRenderer.CreateIndexBuffer(indexData, gSurf);
      } else if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER) {

        // Try to merge buffers to a big buffer.
        // TODO: check how to use multi buffers.
        if (componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
          LOG_E(kTAG, "Vertex data format is not supported.");
          continue;
        }

        assert(accessorType >= TINYGLTF_TYPE_VEC2 && accessorType <= TINYGLTF_TYPE_VEC4);
        gSurf->mVertexCount = bufferView.byteLength / (sizeof(float) * accessorType);
        gSurf->mInstanceCount = gSurf->mIndexCount / 3;
        gSurf->mItemSize = 3;  //accessorType; //bufferView.byteStride / 4; //3
        gSurf->mVertexInput = RenderSurface::VertexInputType_Pos3Normal3Tangent4UV2;
        std::vector<float> vertexData(bufferView.byteLength / sizeof(float));
        memcpy(vertexData.data(), buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);
        gRenderer.CreateVertexBuffer(vertexData, gSurf);
      } else {
        LOG_W(kTAG, "Unsupported buffer view target type.");
      }
    } else {
      // TODO: confirm if it works.
      const auto accessor = model.accessors[sparseAccessor];
      // copy the buffer to a temporary one for sparse patching
      unsigned char *tmpBuffer = new unsigned char[bufferView.byteLength];
      memcpy(tmpBuffer, buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);

      const size_t sizeOfObjectInBuffer = ComponentTypeByteSize(accessor.componentType);
      const size_t sizeOfSparseIndices =
              ComponentTypeByteSize(accessor.sparse.indices.componentType);
      const auto &indicesBufferView =
              model.bufferViews[accessor.sparse.indices.bufferView];
      const auto &indicesBuffer = model.buffers[indicesBufferView.buffer];
      const auto &valuesBufferView =
              model.bufferViews[accessor.sparse.values.bufferView];
      const auto &valuesBuffer = model.buffers[valuesBufferView.buffer];
      for (size_t sparseIndex = 0; sparseIndex < accessor.sparse.count; ++sparseIndex) {
        int index = 0;

        switch (accessor.sparse.indices.componentType) {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            index = (int) *(
                    unsigned char *) (indicesBuffer.data.data() +
                                      indicesBufferView.byteOffset +
                                      accessor.sparse.indices.byteOffset +
                                      (sparseIndex * sizeOfSparseIndices));
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            index = (int) *(
                    unsigned short *) (indicesBuffer.data.data() +
                                       indicesBufferView.byteOffset +
                                       accessor.sparse.indices.byteOffset +
                                       (sparseIndex * sizeOfSparseIndices));
            break;
          case TINYGLTF_COMPONENT_TYPE_INT:
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            index = (int) *(
                    unsigned int *) (indicesBuffer.data.data() +
                                     indicesBufferView.byteOffset +
                                     accessor.sparse.indices.byteOffset +
                                     (sparseIndex * sizeOfSparseIndices));
            break;
        }
        LOG_I(kTAG, "updating sparse data at index  : %d", index);

        const unsigned char *readFrom =
                valuesBuffer.data.data() +
                (valuesBufferView.byteOffset +
                 accessor.sparse.values.byteOffset) +
                (sparseIndex * (sizeOfObjectInBuffer * accessor.type));
        unsigned char *writeTo =
                tmpBuffer + index * (sizeOfObjectInBuffer * accessor.type);
        memcpy(writeTo, readFrom, sizeOfObjectInBuffer * accessor.type);
      }
      delete[] tmpBuffer;
    }
  }

  // create texture
  if (model.images.size()) {
    const Image& image = model.images[0];
    gRenderer.CreateTextureFromBuffer((const char*)image.image.data(), image.width, image.height, image.component,
                                      gSurf);
  }
}

bool InitVulkan(android_app* app) {
  using namespace gfx_math;

  if (!gRenderer.Init(app, kTAG)) {
    return false;
  }

  // 8 vertex
  // create vertex / index buffer
  const std::vector<float> vertexData = {
      // Front face
      -0.5, -0.5, -0.5,
      0.5, -0.5, -0.5,
      0.5, 0.5, -0.5,
      -0.5, 0.5, -0.5,

      // Back face
      -0.5, -0.5, 0.5,
      -0.5, 0.5, 0.5,
      0.5, 0.5, 0.5,
      0.5, -0.5, 0.5,
  };

  const std::vector<uint16_t> indexData = {
      0, 1, 2,  0, 2, 3,
      4, 5, 6,  4, 6, 7,
      5, 3, 2,  5, 2, 6,
      4, 7, 0,  7, 1, 0,
      7, 6, 2,  7, 2, 1,
      0, 5, 4,  0, 3, 5
  };

  struct UniformBufferObject {
    Matrix4x4f mvpMtx;
  };

  // load glTF files by path
  // create surfaces and textures internally.
  // class gRenderer->loadModel return model
  Model model;
  TinyGLTF loader;
  std::string warn, err;
  const std::string inputFileName = Platform::GetExternalDirPath() + "assets/models/Cube/Cube.gltf";
  const std::string ext = GetFilePathExtension(inputFileName);

  bool result = false;
  if (ext == "glb") {
    result = loader.LoadBinaryFromFile(&model, &err, &warn, inputFileName.c_str());
  } else if (ext == "gltf") {
    result = loader.LoadASCIIFromFile(&model, &err, &warn, inputFileName.c_str());
  }

  if (!warn.empty()) {
    LOG_W(kTAG, "Loading model warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    LOG_E(kTAG, "Loading model err: %s\n", err.c_str());
  }

  assert(result && "load gltf model failed.");

  // DBG
  PrintNodes(model.scenes[model.defaultScene > -1 ? model.defaultScene : 0]);

  SetupMeshState(model);
  gRenderer.CreateUniformBuffer(sizeof(UniformBufferObject), gSurf);

  // CreateDescriptorSetLayout needs to be after CreateTextureFromFilZe and CreateUniformBuffer
  gRenderer.CreateDescriptorSetLayout(gSurf);
  gRenderer.CreateGraphicsPipeline("shaders/uniform.vert.spv",
                                   "shaders/uniform.frag.spv", gSurf);
  gRenderer.CreateDescriptorSet(sizeof(UniformBufferObject), gSurf);
  gSurf->mTransformMatrix.Translate(0, 0, -10);

  gRenderer.AddSurface(gSurf);
  gRenderer.ConstructRenderPass();

  return true;
}

bool IsVulkanReady() {
  return gRenderer.IsReady();
}

void TerminateVulkan() {
  gRenderer.Terminate();
}

bool VulkanRenderFrame() {
  gSurf->mTransformMatrix.RotateY(DegreesToRadians(3.0f));
  gRenderer.RenderFrame();
  return true;
}