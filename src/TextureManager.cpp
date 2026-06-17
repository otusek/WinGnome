#include "TextureManager.hpp"

#include "paths.h"
#include "svg_raster.h"

#include <algorithm>
#include <cctype>

namespace wingnome {

namespace {

bool endsWithIgnoreCase(const std::string& value, const char* suffix) {
    const std::string suf = suffix;
    if (value.size() < suf.size()) return false;
    const size_t start = value.size() - suf.size();
    for (size_t i = 0; i < suf.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(value[start + i])) !=
            std::tolower(static_cast<unsigned char>(suf[i])))
            return false;
    }
    return true;
}

}  // namespace

TextureManager& TextureManager::instance() {
    static TextureManager mgr;
    return mgr;
}

bool TextureManager::has(const std::string& assetKey) const {
    return textures_.find(assetKey) != textures_.end();
}

bool TextureManager::loadInto(sf::Texture& texture, const std::filesystem::path& path) {
    if (endsWithIgnoreCase(path.string(), ".svg")) {
        sf::Image image;
        if (!rasterizeSvgFile(path, kSvgRasterSize, image)) return false;
        if (!texture.loadFromImage(image)) return false;
    } else if (!texture.loadFromFile(path.string())) {
        return false;
    }

    texture.setSmooth(true);
    return true;
}

const sf::Texture* TextureManager::get(const std::string& assetKey) {
    if (failed_.count(assetKey)) return nullptr;

    const auto cached = textures_.find(assetKey);
    if (cached != textures_.end()) return &cached->second;

    const std::filesystem::path path = findAssetFile(assetKey);
    sf::Texture texture;
    if (!loadInto(texture, path)) {
        failed_[assetKey] = true;
        return nullptr;
    }

    auto [it, _] = textures_.emplace(assetKey, std::move(texture));
    return &it->second;
}

}  // namespace wingnome
