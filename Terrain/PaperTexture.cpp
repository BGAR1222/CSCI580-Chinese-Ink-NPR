#include "PaperTexture.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
float smooth(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

float sampleGrid(const std::vector<float>& grid, int width, int height,
                 float u, float v)
{
    const float x = u * width;
    const float y = v * height;
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const float tx = smooth(x - ix);
    const float ty = smooth(y - iy);
    const auto at = [&](int gx, int gy) {
        return grid[(gy % height) * width + gx % width];
    };
    const float a = at(ix, iy) * (1.0f - tx) + at(ix + 1, iy) * tx;
    const float b = at(ix, iy + 1) * (1.0f - tx) + at(ix + 1, iy + 1) * tx;
    return a * (1.0f - ty) + b * ty;
}

struct UploadState
{
    GLint binding;
    GLint alignment;
    UploadState()
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    ~UploadState()
    {
        glBindTexture(GL_TEXTURE_2D, binding);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
};
}

std::unique_ptr<Texture> createPaperTexture(int size, unsigned int seed)
{
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (size <= 0 || size > maxSize)
        throw std::invalid_argument("Paper texture size exceeds the OpenGL limit.");

    std::mt19937 generator(seed);
    const auto randomValue = [&]() {
        return static_cast<float>(generator() >> 8) / 16777216.0f;
    };
    std::vector<float> broad(8 * 8);
    std::vector<float> fibers(64 * 8);
    for (float& value : broad) value = randomValue();
    for (float& value : fibers) value = randomValue();

    const std::size_t count = static_cast<std::size_t>(size) * size;
    std::vector<float> pixels(count);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float u = (x + 0.5f) / size;
            const float v = (y + 0.5f) / size;
            const float cloud = sampleGrid(broad, 8, 8, u, v) - 0.5f;
            const float fiber = sampleGrid(fibers, 64, 8, u, v) - 0.5f;
            const float grain = randomValue() - 0.5f;
            pixels[static_cast<std::size_t>(y) * size + x] =
                0.5f + 0.12f * cloud + 0.06f * fiber + 0.025f * grain;
        }
    }

    std::vector<const float*> rows(size);
    for (int y = 0; y < size; ++y)
        rows[y] = pixels.data() + static_cast<std::size_t>(y) * size;

    // Support odd widths without changing the caller's GL state.
    UploadState restore;
    return std::unique_ptr<Texture>(new Texture(size, size, rows.data()));
}
