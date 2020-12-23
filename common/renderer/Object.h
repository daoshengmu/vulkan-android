//
// Created by Daosheng Mu on 8/9/20.
//

#ifndef VULKAN_OBJECT_H
#define VULKAN_OBJECT_H

#include <memory>
#include <unordered_map>
#include "component/Component.h"

class Object {

public:
  void AddComponent(const std::shared_ptr<Component>& aComp);
//private:
  // ComponentType, Compent
  std::unordered_map<uint, std::shared_ptr<Component>> mComponents;
};


#endif //VULKAN_OBJECT_H
