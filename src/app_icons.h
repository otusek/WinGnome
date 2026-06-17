#pragma once

#include "platform.h"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace wingnome {

class AppIconCache {
public:
    sf::Texture* get(const std::wstring& path, int size);
    void clear();

private:
    struct Key {
        std::wstring path;
        int size{0};
        bool operator==(const Key& o) const { return size == o.size && path == o.path; }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<std::wstring>{}(k.path) ^ (static_cast<size_t>(k.size) << 1);
        }
    };

    std::unordered_map<Key, sf::Texture, KeyHash> cache_;
};

std::wstring resolveAppPath(const std::wstring& path);
bool loadIconTexture(const std::wstring& path, int size, sf::Texture& out);

}  // namespace wingnome
