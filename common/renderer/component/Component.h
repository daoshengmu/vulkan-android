//
// Created by Daosheng Mu on 8/9/20.
//

#ifndef VULKANANDROID_COMPONENT_H
#define VULKANANDROID_COMPONENT_H

enum ComponentType : int {
  GEOMETRY_COMPONENT,   // Component which includes vertex and index buffer
  TRANSFORM_COMPONENT,  // Component which includes position and orientation info
  MATERIAL_COMPONENT    // Component which includes shaders and textures.
};

class Component {

public:
  ComponentType mType;
};


#endif //VULKANANDROID_COMPONENT_H
