#include "common.h"
#include "cu/load.h"

#include <fstream>

namespace cu
{
    // TODO: Throw exceptions when load fails
    GlTexture loadTextureFromDdsFile(const std::string & filepath)
    {
        std::ifstream in(filepath.c_str(), std::ifstream::binary);
        if (!in) throw LoadFailure("File not found: " + filepath);

        // Read the file header (FourCC + DDSURFACEDESC2)
        struct { char filecode[4]; uint32_t _unused0[2], height, width, _unused1[2], mips, _unused2[13]; char format[4], _unused3[40]; } dds;
        in.read((char *)&dds, sizeof(dds));
        if (strncmp(dds.filecode, "DDS ", 4) != 0) throw LoadFailure("Not a DDS file: " + filepath);

        // Check the format of the image
        GLuint format;
        if (strncmp(dds.format, "DXT1", 4) == 0) { format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; }
        else if (strncmp(dds.format, "DXT3", 4) == 0) { format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; }
        else if (strncmp(dds.format, "DXT5", 4) == 0) { format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; }
        else throw LoadFailure("Unsupported format ("+std::string(dds.format,dds.format+4)+"): " + filepath);

        // Calculate the size of all mips in the file
        size_t size = 0, width = dds.width, height = dds.height, blockSize = format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16;
        for (size_t mip = 0; mip<dds.mips; ++mip)
        {
            size += ((width + 3) / 4)*((height + 3) / 4)*blockSize;
            width = std::max(width / 2, size_t(1));
            height = std::max(height / 2, size_t(1));
        }

        // Read the remaining contents of the file into a buffer
        std::vector<GLubyte> pixels(size);
        in.read((char *)pixels.data(), size);
        in.close();

        // Pass the compressed texture data to OpenGL
        return GlTexture(format, {dds.width, dds.height}, dds.mips, pixels.data());
    }
}