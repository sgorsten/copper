// json module: Provides support for loading assets from standard formats
#ifndef COPPER_LOAD_H
#define COPPER_LOAD_H

#include "draw.h"

namespace cu
{
    struct LoadFailure : std::runtime_error { LoadFailure(const std::string & what) : runtime_error(what) {} };

    GlTexture loadTextureFromDdsFile(const std::string & filepath);
}

#endif