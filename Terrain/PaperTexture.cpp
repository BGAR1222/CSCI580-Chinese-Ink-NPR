#include "PaperTexture.h"

#include <random>
#include <stdexcept>
#include <vector>

std::unique_ptr<Texture> createPaperTexture(
    int size,
    unsigned int seed
)
{
    if (size <= 0)
        throw std::invalid_argument("Paper texture size must be positive.");

    // RNG with a fixed seed
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> randomValue(0.0f, 1.0f);

    // Store the grayscale pixels in array
    const size_t pixelCount = static_cast<size_t>(size) * size;
    std::vector<float> pixels(pixelCount);

    for (size_t i = 0; i < pixelCount; ++i)
    {
        float noise = randomValue(generator);

        // remap to get low-contrast grain
        pixels[i] = 0.5f + (noise - 0.5f) * 0.1f;
    }

    std::vector<const float*> rows(size);

    for (int y = 0; y < size; ++y)
    {
        rows[y] = pixels.data() + static_cast<size_t>(y) * size;
    }

    // Create the GPU texture
    return std::unique_ptr<Texture>(
        new Texture(size, size, rows.data())
    );
}