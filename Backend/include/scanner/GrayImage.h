#pragma once
// A plain 8-bit grayscale picture (row-major). The scanner works on this type only, so the
// decoder has no dependency on Qt or OpenCV and is easy to unit-test.
#include <cstdint>
#include <vector>

struct GrayImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels; // width * height values, 0 = black, 255 = white

    bool empty() const { return width <= 0 || height <= 0 || pixels.size() != static_cast<std::size_t>(width) * height; }
    std::uint8_t at(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }
};
