//
// Created by Daosheng Mu on 8/9/20.
//

#ifndef VULKAN_MATERIAL_H
#define VULKAN_MATERIAL_H

#include <string>
#include "Component.h"


class Material : public  Component {
public:
  Material() { mType = MATERIAL_COMPONENT; }
  //void SetupShaderByFileName(const std::string& aVSName, const std::string& aFSName);

  std::string mVertexShader;
  std::string mFragShader;
};


#endif //VULKAN_MATERIAL_H
