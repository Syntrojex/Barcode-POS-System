// Unit tests for the barcode decoder (pure C++, no camera): pictures are drawn in memory.
#include "decoder.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAILED line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static std::string modulesFor(const std::string &d)
{
    std::string m = "101";
    for (int i = 0; i < 4; ++i) m += Decoder::patterns[d[i] - '0'];
    m += "01010";
    for (int i = 4; i < 8; ++i)
        for (const char *p = Decoder::patterns[d[i] - '0']; *p; ++p) m += (*p == '1') ? '0' : '1';
    return m + "101";
}

// Draw the barcode `scale` pixels per module, with quiet zones, inside a larger picture.
static GrayImage draw(const std::string &digits, int scale, int margin, bool upsideDown = false, bool sideways = false, int noise = 0)
{
    std::string m = modulesFor(digits);
    if (upsideDown) m.assign(m.rbegin(), m.rend());
    const int w = static_cast<int>(m.size()) * scale + 2 * margin, h = 120;
    GrayImage g;
    g.width = sideways ? h : w;
    g.height = sideways ? w : h;
    g.pixels.assign(static_cast<std::size_t>(g.width) * g.height, 235);
    std::srand(7);
    for (int x = 0; x < w; ++x)
    {
        const int mi = (x - margin) / scale;
        const bool dark = x >= margin && mi >= 0 && mi < static_cast<int>(m.size()) && m[static_cast<std::size_t>(mi)] == '1';
        for (int y = 20; y < 100; ++y)
        {
            int v = dark ? 20 : 235;
            if (noise) v += std::rand() % (2 * noise + 1) - noise;
            v = v < 0 ? 0 : (v > 255 ? 255 : v);
            const int px = sideways ? y : x, py = sideways ? x : y;
            g.pixels[static_cast<std::size_t>(py) * g.width + px] = static_cast<std::uint8_t>(v);
        }
    }
    return g;
}

int main()
{
    Decoder d;
    std::string out;
    CHECK(Decoder::hasValidCheckDigit("40001029"));
    CHECK(!Decoder::hasValidCheckDigit("40001028"));

    for (const char *code : {"40001029", "40002149", "40003511", "96385074", "00000000"})
    {
        if (!Decoder::hasValidCheckDigit(code)) continue;
        CHECK(d.decodeCropping(draw(code, 3, 20), out) && out == code);
        CHECK(d.decodeCropping(draw(code, 2, 30), out) && out == code);
        CHECK(d.decodeCropping(draw(code, 5, 10), out) && out == code);
        CHECK(d.decodeCropping(draw(code, 3, 20, true), out) && out == code);        // upside down
        CHECK(d.decodeCropping(draw(code, 3, 20, false, true), out) && out == code); // sideways
        CHECK(d.decodeCropping(draw(code, 4, 20, false, false, 18), out) && out == code); // camera noise
    }
    // barcode only in the lower part of the picture: found when the aiming line is moved onto it
    {
        GrayImage g = draw("40001029", 3, 20);
        for (int y = 0; y < 60; ++y)                       // blank out the top half of the bars
            for (int x = 0; x < g.width; ++x) g.pixels[static_cast<std::size_t>(y) * g.width + x] = 235;
        CHECK(d.decodeCropping(g, out, 0.8) && out == "40001029");
    }
    // wrong check digit: not accepted, but reported
    CHECK(!d.decodeCropping(draw("40001028", 3, 20), out));
    CHECK(d.isBadCheck());
    // blank picture
    GrayImage blank; blank.width = 200; blank.height = 100; blank.pixels.assign(20000, 200);
    CHECK(!d.decodeCropping(blank, out));
    CHECK(!d.decodeCropping(GrayImage{}, out));

    if (failures) { std::printf("%d scanner test(s) FAILED\n", failures); return 1; }
    std::printf("scanner tests OK\n");
    return 0;
}
