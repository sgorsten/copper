#include "common.h"
#include "cu/draw.h"
#include <cassert>

using namespace cu;

GlSampler::GlSampler(GLenum magFilter, GLenum minFilter, GLenum wrapMode, bool isShadow) : GlSampler()
{
    glGenSamplers(1,&obj);
    glSamplerParameteri(obj, GL_TEXTURE_MAG_FILTER, magFilter);
    glSamplerParameteri(obj, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(obj, GL_TEXTURE_WRAP_S, wrapMode);
    glSamplerParameteri(obj, GL_TEXTURE_WRAP_T, wrapMode);
    if (isShadow)
    {
        glSamplerParameteri(obj, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(obj, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
}

GlTexture::GlTexture(GLenum format, uint2 dims, int mips, const void * pixels) : GlTexture()
{
    bool compressed = false;
    uint2 blockSize; int blockBytes; GLenum pixelFormat; GLenum pixelType;
    switch (format)
    {
    case GL_RGB:  case GL_BGR:  blockSize = uint2(1, 1); blockBytes = 3; pixelFormat = GL_RGBA; pixelType = GL_UNSIGNED_BYTE; break;
    case GL_RGBA: case GL_BGRA: blockSize = uint2(1, 1); blockBytes = 4; pixelFormat = GL_RGBA; pixelType = GL_UNSIGNED_BYTE; break;
    case GL_DEPTH_COMPONENT: blockSize = uint2(1, 1); blockBytes = 4; pixelFormat = GL_DEPTH_COMPONENT; pixelType = GL_FLOAT; break;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: compressed = true; blockSize = uint2(4, 4); blockBytes = 8; break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: compressed = true; blockSize = uint2(4, 4); blockBytes = 16; break;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: compressed = true; blockSize = uint2(4, 4); blockBytes = 16; break;
    default: throw std::runtime_error("GlTexture(...) - Unexpected format");
    }

    auto bytes = reinterpret_cast<const uint8_t *>(pixels);
    glGenTextures(1, &obj);
    glBindTexture(GL_TEXTURE_2D, obj);
    uint2 mipSize = dims;
    for (GLint mip = 0; mip<mips; ++mip)
    {
        const auto blocks = (mipSize+blockSize-1U)/blockSize;
        const auto mipBytes = blocks.x*blocks.y*blockBytes;
        if (compressed) glCompressedTexImage2D(GL_TEXTURE_2D, mip, format, mipSize.x, mipSize.y, 0, mipBytes, bytes);
        else glTexImage2D(GL_TEXTURE_2D, mip, format, mipSize.x, mipSize.y, 0, pixelFormat, pixelType, bytes);
        bytes += mipBytes;
        mipSize = (mipSize + uint2(1, 1)) / 2U;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

GlFramebuffer::GlFramebuffer(uint2 dims, GLenum colorFormat, size_t numColorTextures, GLenum depthStencilFormat, bool hasDepthStencilTexture) : GlFramebuffer(dims)
{
    glGenFramebuffers(1, &obj);
    glBindFramebuffer(GL_FRAMEBUFFER, obj);

    std::vector<GLenum> drawBufs;
    for (size_t i = 0; i < numColorTextures; ++i)
    {
        textures.emplace_back(colorFormat, dims, 1, nullptr);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures.back().obj, 0);
        drawBufs.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    if (!numColorTextures) glDrawBuffer(GL_NONE);

    if (depthStencilFormat)
    {
        if (hasDepthStencilTexture)
        {
            textures.emplace_back(depthStencilFormat, dims, 1, nullptr);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textures.back().obj, 0);
        }
        else
        {
            glGenRenderbuffers(1,&rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, depthStencilFormat, dims.x, dims.y);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }
    }
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Framebuffer incomplete!");
    glDrawBuffers(drawBufs.size(), drawBufs.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GlMesh::~GlMesh()
{
    glDeleteVertexArrays(1,&vertArray);
    glDeleteBuffers(1,&elemBuf);
    glDeleteBuffers(1,&arrBuf);
}

GlMesh & GlMesh::operator = (GlMesh && r) 
{
    std::swap(vertArray, r.vertArray); 
    std::swap(arrBuf, r.arrBuf); 
    std::swap(elemBuf, r.elemBuf); 
    numVerts = r.numVerts; 
    numElems = r.numElems;
    emode = r.emode;
    itype = r.itype; 
    return *this; 
}

void GlMesh::setElements(const void * elements, size_t indexSize, size_t elemSize, size_t numElements, GLenum usage)
{
    assert(indexSize == 1 || indexSize == 2 || indexSize == 4);
    assert((elemSize == 2 || elemSize == 3) && "elemSize must be 2 (lines) or 3 (triangles)");

    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!elemBuf) glGenBuffers(1, &elemBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize*elemSize*numElements, elements, usage);
    glBindVertexArray(0);

    emode = elemSize == 3 ? GL_TRIANGLES : GL_LINES;
    itype = indexSize == 4 ? GL_UNSIGNED_INT : indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
    numElems = elemSize * numElements;
}

void GlMesh::setVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage)
{
    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*numVertices, vertices, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    numVerts = numVertices;
}

void GlMesh::setAttribute(int loc, std::nullptr_t)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);
    glDisableVertexAttribArray(loc);
    glBindVertexArray(0);
}

void GlMesh::setAttribute(int loc, int size, GLenum type, bool normalized, size_t stride, ptrdiff_t offset)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, size, type, normalized ? GL_TRUE : GL_FALSE, stride, reinterpret_cast<const GLvoid *>(offset));

    glBindVertexArray(0);
}

void GlMesh::draw() const
{
    if (!vertArray || numVerts == 0) return;
    glBindVertexArray(vertArray);
    if (numElems > 0) glDrawElements(emode, numElems, itype, 0);
    else glDrawArrays(emode, 0, numVerts);
}

GlShader::GlShader(GLenum type, const char * source) : GlShader()
{
    obj = glCreateShader(type);
    glShaderSource(obj, 1, &source, nullptr);
    glCompileShader(obj);

    GLint status; glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length; glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::string infolog(length, 0); glGetShaderInfoLog(obj, infolog.size(), nullptr, &infolog[0]);
        throw std::runtime_error("GLSL compile error: " + infolog);
    }
}

GlProgram::GlProgram(GlShader _vs, GlShader _fs) : GlProgram()
{
    vs = std::move(_vs);
    fs = std::move(_fs);

    obj = glCreateProgram();
    glAttachShader(obj, vs.obj);
    glAttachShader(obj, fs.obj);
    glLinkProgram(obj);

    GLint status; glGetProgramiv(obj, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length; glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::string infolog(length, 0); glGetProgramInfoLog(obj, infolog.size(), nullptr, &infolog[0]);
        throw std::runtime_error("GLSL link error: " + infolog);
    }

    // Obtain information about the active uniform blocks in the program
    GLint numBlocks; glGetProgramiv(obj, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
    for (GLuint i = 0; i < numBlocks; ++i)
    {
        GLint binding, dataSize, nameLength;
        glGetActiveUniformBlockiv(obj, i, GL_UNIFORM_BLOCK_BINDING, &binding);
        glGetActiveUniformBlockiv(obj, i, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
        glGetActiveUniformBlockiv(obj, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);

        std::string name(nameLength - 1, ' ');
        glGetActiveUniformBlockName(obj, i, nameLength, nullptr, &name[0]);

        BlockDesc bl;
        bl.name = name;
        bl.binding = binding;
        bl.dataSize = dataSize;
        desc.blocks.push_back(bl);
    }

    // Obtain information about the active uniforms in the program
    GLint numUniforms; glGetProgramiv(obj, GL_ACTIVE_UNIFORMS, &numUniforms);
    for (GLuint i = 0; i < numUniforms; ++i)
    {
        GLint type, size, nameLength, blockIndex;
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_TYPE, &type);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_NAME_LENGTH, &nameLength);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_BLOCK_INDEX, &blockIndex);

        UniformDesc un;
        un.name.resize(nameLength-1);
        un.size = uint3(1,1,size);
        glGetActiveUniformName(obj, i, nameLength, nullptr, &un.name[0]);

        switch (type)
        {
        case GL_FLOAT: un.type = UniformDesc::Float; break;
        case GL_FLOAT_VEC2: un.type = UniformDesc::Float; un.size.x = 2; break;
        case GL_FLOAT_VEC3: un.type = UniformDesc::Float; un.size.x = 3; break;
        case GL_FLOAT_VEC4: un.type = UniformDesc::Float; un.size.x = 4; break;

        case GL_DOUBLE: un.type = UniformDesc::Double; break;
        case GL_DOUBLE_VEC2: un.type = UniformDesc::Double; un.size.x = 2; break;
        case GL_DOUBLE_VEC3: un.type = UniformDesc::Double; un.size.x = 3; break;
        case GL_DOUBLE_VEC4: un.type = UniformDesc::Double; un.size.x = 4; break;

        case GL_INT: un.type = UniformDesc::Int; break;
        case GL_INT_VEC2: un.type = UniformDesc::Int; un.size.x = 2; break;
        case GL_INT_VEC3: un.type = UniformDesc::Int; un.size.x = 3; break;
        case GL_INT_VEC4: un.type = UniformDesc::Int; un.size.x = 4; break;

        case GL_UNSIGNED_INT: un.type = UniformDesc::UInt; break;
        case GL_UNSIGNED_INT_VEC2: un.type = UniformDesc::UInt; un.size.x = 2; break;
        case GL_UNSIGNED_INT_VEC3: un.type = UniformDesc::UInt; un.size.x = 3; break;
        case GL_UNSIGNED_INT_VEC4: un.type = UniformDesc::UInt; un.size.x = 4; break;

        case GL_BOOL: un.type = UniformDesc::Bool; break;
        case GL_BOOL_VEC2: un.type = UniformDesc::Bool; un.size.x = 2; break;
        case GL_BOOL_VEC3: un.type = UniformDesc::Bool; un.size.x = 3; break;
        case GL_BOOL_VEC4: un.type = UniformDesc::Bool; un.size.x = 4; break;

        case GL_FLOAT_MAT2: un.type = UniformDesc::Float; un.size.x = un.size.y = 2; break;
        case GL_FLOAT_MAT3: un.type = UniformDesc::Float; un.size.x = un.size.y = 3; break;
        case GL_FLOAT_MAT4: un.type = UniformDesc::Float; un.size.x = un.size.y = 4; break;
        case GL_FLOAT_MAT2x3: un.type = UniformDesc::Float; un.size.x = 3; un.size.y = 2; break;
        case GL_FLOAT_MAT2x4: un.type = UniformDesc::Float; un.size.x = 4; un.size.y = 2; break;
        case GL_FLOAT_MAT3x2: un.type = UniformDesc::Float; un.size.x = 2; un.size.y = 3; break;
        case GL_FLOAT_MAT3x4: un.type = UniformDesc::Float; un.size.x = 4; un.size.y = 3; break;
        case GL_FLOAT_MAT4x2: un.type = UniformDesc::Float; un.size.x = 2; un.size.y = 4; break;
        case GL_FLOAT_MAT4x3: un.type = UniformDesc::Float; un.size.x = 3; un.size.y = 4; break;

        case GL_DOUBLE_MAT2: un.type = UniformDesc::Double; un.size.x = un.size.y = 2; break;
        case GL_DOUBLE_MAT3: un.type = UniformDesc::Double; un.size.x = un.size.y = 3; break;
        case GL_DOUBLE_MAT4: un.type = UniformDesc::Double; un.size.x = un.size.y = 4; break;
        case GL_DOUBLE_MAT2x3: un.type = UniformDesc::Double; un.size.x = 3; un.size.y = 2; break;
        case GL_DOUBLE_MAT2x4: un.type = UniformDesc::Double; un.size.x = 4; un.size.y = 2; break;
        case GL_DOUBLE_MAT3x2: un.type = UniformDesc::Double; un.size.x = 2; un.size.y = 3; break;
        case GL_DOUBLE_MAT3x4: un.type = UniformDesc::Double; un.size.x = 4; un.size.y = 3; break;
        case GL_DOUBLE_MAT4x2: un.type = UniformDesc::Double; un.size.x = 2; un.size.y = 4; break;
        case GL_DOUBLE_MAT4x3: un.type = UniformDesc::Double; un.size.x = 3; un.size.y = 4; break;

        default:
            if (blockIndex != -1) throw std::runtime_error("Unsupported type outside of uniform block in GLSL program: "+un.name);
            else
            {
                SamplerDesc samp;
                samp.name = un.name;
                glGetUniformiv(obj, glGetUniformLocation(obj, samp.name.c_str()), (GLint*)&samp.binding);
                desc.samplers.push_back(samp);
            }
            continue;
        }
        
        if(blockIndex == -1) continue; // We do not currently support non-samplers outside of uniform blocks

        // For uniforms that belong to a uniform block, determine their positioning and layout
        GLint offset, arrayStride, matrixStride, isRowMajor;
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_OFFSET, &offset);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_ARRAY_STRIDE, &arrayStride);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_MATRIX_STRIDE, &matrixStride);
        glGetActiveUniformsiv(obj, 1, &i, GL_UNIFORM_IS_ROW_MAJOR, &isRowMajor);

        un.offset = offset;
        un.stride = uint3(un.type == UniformDesc::Double ? 8 : 4, matrixStride > 0 ? matrixStride : 0, arrayStride > 0 ? arrayStride : 0);
        if (isRowMajor) std::swap(un.stride.x, un.stride.y);
        desc.blocks[blockIndex].uniforms.push_back(un);
    }
}