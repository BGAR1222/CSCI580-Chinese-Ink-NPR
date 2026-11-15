#ifndef PAPER_TEXTURE_H
#define PAPER_TEXTURE_H

#include <memory>
#include "Texture.h"

std::unique_ptr<Texture> createPaperTexture(
    int size = 256,
    unsigned int seed = 580
);

#endif