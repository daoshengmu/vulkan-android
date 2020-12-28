//
// Created by Daosheng Mu on 12/24/20.
//

#ifndef VULKANANDROID_CUBE_H
#define VULKANANDROID_CUBE_H

#include "RenderSurface.h"
#include <vector>

class Cube : public  RenderSurface {

public:
  Cube();

  const std::vector<float>& GetVertexData() const;
  const std::vector<uint16_t>& GetIndexData() const;

private:
  //static bool mInited;
  static std::vector<float> mCubeVertexData;
  static std::vector<uint16_t> mCubeIndexData;
};


#endif //VULKANANDROID_CUBE_H
