#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>

namespace wingnome {

// Rasterize an SVG file once into an RGBA image (square-ish, max edge = rasterSize).
bool rasterizeSvgFile(const std::filesystem::path& path, unsigned rasterSize, sf::Image& out);

}  // namespace wingnome
