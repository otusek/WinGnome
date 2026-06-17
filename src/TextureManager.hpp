#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <map>
#include <string>

namespace wingnome {

// Singleton cache: each SVG/PNG asset is rasterized or loaded at most once per process.
class TextureManager {
public:
    static TextureManager& instance();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // assetKey e.g. "icons/volume.svg" — returns nullptr when missing or failed.
    const sf::Texture* get(const std::string& assetKey);

    bool has(const std::string& assetKey) const;

private:
    TextureManager() = default;

    bool loadInto(sf::Texture& texture, const std::filesystem::path& path);

    static constexpr unsigned kSvgRasterSize = 64;

    std::map<std::string, sf::Texture> textures_;
    std::map<std::string, bool> failed_;
};

}  // namespace wingnome
