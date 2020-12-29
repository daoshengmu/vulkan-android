//
// Created by Daosheng Mu on 12/26/20.
//

#ifndef VULKANANDROID_COMMONUTILS_PLATFORM_H
#define VULKANANDROID_COMMONUTILS_PLATFORM_H

#include <string>

class Platform {
public:
  static void SetExternalDirPath(const std::string& aPath);
  static const std::string& GetExternalDirPath();

private:
  static std::string mExternalDir;

};

#endif //VULKANANDROID_COMMONUTILS_PLATFORM_H
