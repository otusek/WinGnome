#include "svg_raster.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include <algorithm>
#include <fstream>
#include <vector>

namespace wingnome {

bool rasterizeSvgFile(const std::filesystem::path& path, unsigned rasterSize, sf::Image& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.empty()) return false;
    data.push_back('\0');

    NSVGimage* image = nsvgParse(data.data(), "px", 96.f);
    if (!image) return false;

    const float base = std::max(image->width, image->height);
    const float scale = base > 0.f ? static_cast<float>(rasterSize) / base : 1.f;
    const int width = std::max(1, static_cast<int>(image->width * scale));
    const int height = std::max(1, static_cast<int>(image->height * scale));

    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    if (!rasterizer) {
        nsvgDelete(image);
        return false;
    }

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    nsvgRasterize(rasterizer, image, 0.f, 0.f, scale, pixels.data(), width, height, width * 4);

    // Bake GNOME symbolic color into a white RGB + alpha mask for fast sprite tinting.
    for (size_t i = 0; i < pixels.size(); i += 4) {
        const unsigned char r = pixels[i];
        const unsigned char g = pixels[i + 1];
        const unsigned char b = pixels[i + 2];
        const unsigned char a = pixels[i + 3];
        const unsigned char alpha = std::max(a, static_cast<unsigned char>((r + g + b) / 3));
        pixels[i] = 255;
        pixels[i + 1] = 255;
        pixels[i + 2] = 255;
        pixels[i + 3] = alpha;
    }

    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);

    out.create(static_cast<unsigned>(width), static_cast<unsigned>(height), pixels.data());
    return true;
}

}  // namespace wingnome
