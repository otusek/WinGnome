#pragma once

#include <filesystem>
#include <string>

namespace wingnome {

std::filesystem::path exeDir();
std::filesystem::path findConfigFile(const wchar_t* filename);
std::filesystem::path findAssetFile(const std::string& relativePath);

}  // namespace wingnome
