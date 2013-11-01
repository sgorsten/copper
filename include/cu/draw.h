// draw module - Contains utilities for 3D rendering (currently just convenient C++ wrappers around GL types)
#ifndef COPPER_DRAW_H
#define COPPER_DRAW_H

#include "math.h"
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>

namespace cu
{
    template<class C, class T> ptrdiff_t fieldOffset(const T C::*field) { return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<const C *>(0)->*field)); }

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
        friend class GlProgram;
        GLuint obj;

        GlTexture(const GlTexture & r) = delete;
        GlTexture & operator = (const GlTexture & r) = delete;
    public:
        GlTexture() : obj() {}
        GlTexture(GLenum format, uint2 dims, int mips, const void * pixels);
        GlTexture(GlTexture && r) : obj(r.obj) { r.obj = 0; }
        ~GlTexture() { glDeleteTextures(1,&obj); }

        GLuint _name() const { return obj; }

        void bind(GLuint unit, const GlSampler & samp) const { glActiveTexture(GL_TEXTURE0 + unit); glBindTexture(GL_TEXTURE_2D, obj); glBindSampler(unit, samp.obj); } // Warning: This command affects global GL state

        GlTexture & operator = (GlTexture && r) { std::swap(obj, r.obj); return *this; }
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

    class GlProgram
    {
        GLuint obj;
        GlShader vs, fs;

        GlProgram(const GlProgram & r) = delete;
        GlProgram & operator = (const GlProgram & r) = delete;
    public:
        GlProgram() : obj() {}
        GlProgram(GlShader vs, GlShader fs);
        GlProgram(GlProgram && r) : obj(r.obj) { r.obj = 0; }
        ~GlProgram() { glDeleteProgram(obj); }

        void use() const { glUseProgram(obj); } // Warning: This command affects global GL state
        template<class T> void uniform(const char * name, const T & val) const { SetUniform(glGetUniformLocation(obj, name), val); }
        GlProgram & operator = (GlProgram && r) { std::swap(obj, r.obj); return *this; }
    };
}

#endif