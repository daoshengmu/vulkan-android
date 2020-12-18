//
// Created by Daosheng Mu on 8/9/20.
//

#include <utils.h>
#include "Object.h"


void Object::AddComponent(const std::shared_ptr<Component>& aComp) {
//  mComponents.insert(aComp.mType, aComp);
 if (!mComponents.insert({aComp->mType, aComp}).second) {
   LOG_I("Object", "A component type is already added.");
 }
//  mComponents[aComp->mType] = aComp;
}