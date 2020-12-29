//
// Created by Daosheng Mu on 12/26/20.
//

#include <filesystem>
#include "Platform.h"

std::string Platform::mExternalDir;

void Platform::SetExternalDirPath(const std::string& aPath) {
  mExternalDir = aPath + "/";
}

const std::string& Platform::GetExternalDirPath() {
  return mExternalDir;
}

//void Platform::CreatePath(const std::string& aPath) {
//  int index = aPath.rfind('/');
//  CreateNewDirectory(aPath.substr(0, index));
//}

//void Platform::CreateNewDirectory(const std::string& aPath)
//{
//  if (!std::__fs::filesystem::is_directory(aPath)) {
//    CreateDirectory(aPath.c_str(), NULL);
//  }
//}