// draw module - Contains utilities for 3D rendering (currently just convenient C++ wrappers around GL types)
#ifndef COPPER_DRAW_H
#define COPPER_DRAW_H

#include "math.h"
#include <vector>
#include <cassert>

#define GLEW_STATIC
#include <GL/glew.h>

namespace cu
{
    template<class C, class T> ptrdiff_t fieldOffset(const T C::*field) { return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<const C *>(0)->*field)); }

    class GlUniformBuffer
    {
        GLuint obj;

        GlUniformBuffer(const GlUniformBuffer & r) = delete;
        GlUniformBuffer & operator = (const GlUniformBuffer & r) = delete;
    public:
        GlUniformBuffer() : obj() {}
        GlUniformBuffer(GlUniformBuffer && r) : obj(r.obj) { r.obj = 0; }
        ~GlUniformBuffer() { glDeleteBuffers(1, &obj); }

        void bind(GLuint index) const { glBindBufferBase(GL_UNIFORM_BUFFER, index, obj); } // Warning: This command affects global GL state

        void setData(const void * data, size_t size, GLenum usage);

        GlUniformBuffer & operator = (GlUniformBuffer && r) { std::swap(obj, r.obj); return *this; }
    };

    class GlSampler
    {
        friend class GlTexture;
        GLuint obj;

        GlSampler(const GlSampler & r) = delete;
        GlSampler & operator = (const GlSampler & r) = delete;
    public:
        GlSampler() : obj() {}
        GlSampler(GLenum magFilter, GLenum minFilter, GLenum wrapMode, bool isShadow);
        GlSampler(GlSampler && r) : obj(r.obj) { r.obj = 0; }
        ~GlSampler() { glDeleteSamplers(1, &obj); }

        GlSampler & operator = (GlSampler && r) { std::swap(obj, r.obj); return *this; }
    };

    class GlTexture
    {
        friend class GlFramebuffer;
        GLuint obj;

        GlTexture(const GlTexture & r) = delete;
        GlTexture & operator = (const GlTexture & r) = delete;
    public:
        GlTexture() : obj() {}
        GlTexture(GLenum format, uint2 dims, int mips, const void * pixels);
        GlTexture(GlTexture && r) : obj(r.obj) { r.obj = 0; }
        ~GlTexture() { glDeleteTextures(1,&obj); }

        void bind(GLuint unit, const GlSampler & samp) const { glActiveTexture(GL_TEXTURE0 + unit); glBindTexture(GL_TEXTURE_2D, obj); glBindSampler(unit, samp.obj); } // Warning: This command affects global GL state

        GlTexture & operator = (GlTexture && r) { std::swap(obj, r.obj); return *this; }
    };

    class GlFramebuffer
    {
        uint2 dims;
        GLuint obj, rbo;
        std::vector<GlTexture> textures;

        GlFramebuffer(const GlFramebuffer & r) = delete;
        GlFramebuffer & operator = (const GlFramebuffer & r) = delete;
    public:
        GlFramebuffer(uint2 dims = uint2()) : dims(dims), obj(), rbo() {}
        GlFramebuffer(uint2 dims, GLenum colorFormat, size_t numColorTextures, GLenum depthStencilFormat, bool hasDepthStencilTexture);
        GlFramebuffer(GlFramebuffer && r) : dims(r.dims), obj(r.obj), rbo(r.rbo), textures(move(r.textures)) { r.obj = r.rbo = 0; }
        ~GlFramebuffer() { glDeleteFramebuffers(1, &obj); glDeleteRenderbuffers(1, &rbo); }

        const uint2 & dimensions() const { return dims; }       
        const GlTexture & texture(size_t index) const { return textures[index]; }
        void bind() const { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj); glViewport(0, 0, dims.x, dims.y); }

        GlFramebuffer & operator = (GlFramebuffer && r) { dims = r.dims; std::swap(obj, r.obj); std::swap(rbo, r.rbo); textures = move(r.textures); return *this; }

        static GlFramebuffer imageBuffer(uint2 dims, GLenum format)                     { return GlFramebuffer(dims, format, 1, 0, false); } // Buffer with one color texture and no depth buffer, suitable for post-processing passes
        static GlFramebuffer shadowBuffer(uint2 dims)                                   { return GlFramebuffer(dims, 0, 0, GL_DEPTH_COMPONENT, true); } // Buffer with depth texture but no color attachments, suitable for shadow maps
        static GlFramebuffer geometryBuffer(uint2 dims, GLenum format, int channels)    { return GlFramebuffer(dims, format, channels, GL_DEPTH_COMPONENT, false); } // Buffer with depth buffer and arbitrary number of color textures, for deferred lighting style G-buffers
    };

    class GlMesh
    {
        GLuint vertArray, arrBuf, elemBuf;
        GLsizei numVerts, numElems;
        GLenum emode, itype;

        GlMesh(const GlMesh & r) = delete;
        GlMesh & operator = (const GlMesh & r) = delete;

        void setAttribList(int loc) {}
        template<class T, class... R> void setAttribList(int loc, T attrib, R... attribs) { setAttribute(loc, attrib); setAttribList(loc+1, attribs...); }
    public:
        GlMesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems(), emode(GL_TRIANGLES), itype() {}
        GlMesh(GlMesh && r) : vertArray(r.vertArray), arrBuf(r.arrBuf), elemBuf(r.elemBuf), numVerts(r.numVerts), numElems(r.numElems), emode(r.emode), itype(r.itype) { r.vertArray = r.arrBuf = r.elemBuf = 0; }
        template<class V, class E, class... R> GlMesh(const std::vector<V> & verts, const std::vector<E> & elems, R... attribs) : GlMesh() { setVertices(verts); setElements(elems); setAttribList(0, attribs...); }
        ~GlMesh();

        void draw() const;

        void setElements(const void * elements, size_t indexSize, size_t elemSize, size_t numElements, GLenum usage);
        void setVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage);
        void setAttribute(int loc, int size, GLenum type, bool normalized, size_t stride, ptrdiff_t offset);
        void setAttribute(int loc, std::nullptr_t);

        template<class I, int N> void setElements(const std::vector<vec<I,N>> & elems, GLenum usage = GL_STATIC_DRAW) { setElements(&elems[0].x, sizeof(I), N, elems.size(), usage); }
        template<class V>        void setVertices(const std::vector<V> & vertices, GLenum usage = GL_STATIC_DRAW) { setVertices(vertices.data(), sizeof(V), vertices.size(), usage); }
        template<class V, int N> void setAttribute(int loc, const vec<float,N> V::*attribute) { setAttribute(loc, N, GL_FLOAT, false, sizeof(V), fieldOffset(attribute)); }
        template<class V, int N> void setAttribute(int loc, const vec<uint8_t,N> V::*attribute, bool normalized = true) { setAttribute(loc, N, GL_UNSIGNED_BYTE, normalized, sizeof(V), fieldOffset(attribute)); }

        GlMesh & operator = (GlMesh && r);
    };

    class GlShader
    {
        friend class GlProgram;
        GLuint obj;

        GlShader(const GlShader & r) = delete;
        GlShader & operator = (const GlShader & r) = delete;
    public:
        GlShader() : obj() {}
        GlShader(GLenum type, const char * source);
        GlShader(GlShader && r) : obj(r.obj) { r.obj = 0; }
        ~GlShader() { glDeleteShader(obj); }

        GlShader & operator = (GlShader && r) { std::swap(obj, r.obj); return *this; }
    };

    // Warning: These commands affect global GL state
    inline void SetUniform(GLint loc, int val) { glUniform1i(loc, val); }
    inline void SetUniform(GLint loc, float val) { glUniform1f(loc, val); }
    inline void SetUniform(GLint loc, const float2 & val) { glUniform2fv(loc, 1, &val.x); }
    inline void SetUniform(GLint loc, const float3 & val) { glUniform3fv(loc, 1, &val.x); }
    inline void SetUniform(GLint loc, const float4 & val) { glUniform4fv(loc, 1, &val.x); }
    inline void SetUniform(GLint loc, const float4x4 & val) { glUniformMatrix4fv(loc, 1, GL_FALSE, &val.x.x); }

    struct SamplerDesc { std::string name; GLuint binding; };
    struct UniformDesc { enum Type { Float, Double, Int, UInt, Bool }; std::string name; GLuint offset; Type type; uint3 size, stride; 
        static void write(uint8_t & dest, Type type, double value)
        {
            switch (type)
            {
            case Float: reinterpret_cast<float &>(dest) = static_cast<float>(value); break;
            case Double: reinterpret_cast<double &>(dest) = value; break;
            case Int: reinterpret_cast<int32_t &>(dest) = static_cast<int32_t>(value); break;
            case UInt: reinterpret_cast<uint32_t &>(dest) = static_cast<uint32_t>(value); break;
            case Bool: reinterpret_cast<int32_t &>(dest) = value ? 1 : 0; break;
            default: assert(false);
            }
        }
        template<class T> void set(uint8_t * buffer, uint3 index, const T & value) const { if (index.x < size.x && index.y < size.y && index.z < size.z) write(buffer[offset + dot(index,stride)], type, static_cast<double>(value)); }
        template<class T, int M, int N> void set(uint8_t * buffer, size_t element, const mat<T, M, N> & value) const { for (int i = 0; i<N; ++i) for (int j = 0; j<M; ++j) set(buffer, uint3(j, i, element), value(i, j)); }
        template<class T, int M> void set(uint8_t * buffer, size_t element, const vec<T, M> & value) const { for (int j = 0; j<M; ++j) set(buffer, uint3(j, 0, element), value[j]); }
        template<class T> void set(uint8_t * buffer, size_t element, const T & value) const { set(buffer, uint3(0, 0, element), value); }
    }; // x = row, y = column, z = array element
    struct BlockDesc { std::string name; GLuint binding, dataSize; std::vector<UniformDesc> uniforms;
        template<class T> void set(uint8_t * buffer, const char * name, size_t element, const T & value) const { for (auto & un : uniforms) if (un.name == name) un.set(buffer, element, value); }
        template<class T> void set(uint8_t * buffer, const char * name, const T & value) const { set(buffer, name, 0, value); }
    };
    struct ProgramDesc { std::vector<BlockDesc> blocks; std::vector<SamplerDesc> samplers; 
        const BlockDesc * block(const char * name) const { for(auto & bl : blocks) if(bl.name == name) return &bl; return nullptr; }
        const SamplerDesc * sampler(const char * name) const { for (auto & samp : samplers) if (samp.name == name) return &samp; return nullptr; }
    };

    class GlProgram
    {
        GLuint obj;
        GlShader vs, fs;
        ProgramDesc desc;

        GlProgram(const GlProgram & r) = delete;
        GlProgram & operator = (const GlProgram & r) = delete;
    public:
        GlProgram() : obj() {}
        GlProgram(GlShader vs, GlShader fs);
        GlProgram(GlProgram && r) : obj(r.obj), vs(std::move(r.vs)), fs(std::move(r.fs)), desc(r.desc) { r.obj = 0; }
        ~GlProgram() { glDeleteProgram(obj); }

        const ProgramDesc & description() const { return desc; }
        const BlockDesc * block(const char * name) const { return desc.block(name); }
        const SamplerDesc * sampler(const char * name) const { return desc.sampler(name); }
        void use() const { glUseProgram(obj); } // Warning: This command affects global GL state

        template<class T> void uniform(const char * name, const T & val) const { SetUniform(glGetUniformLocation(obj, name), val); }
        GlProgram & operator = (GlProgram && r) { std::swap(obj, r.obj); vs=std::move(r.vs); fs=std::move(r.fs); desc=r.desc; return *this; }
    };
}

#endif