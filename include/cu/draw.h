#ifndef COPPER_DRAW_H
#define COPPER_DRAW_H

#include "math.h"
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>

namespace cu
{
    class GlMesh
    {
        GLuint vertArray, arrBuf, elemBuf;
        GLsizei numVerts, numElems;

        GlMesh(const GlMesh & r) = delete;
        GlMesh & operator = (const GlMesh & r) = delete;
    public:
        GlMesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems() {}
        GlMesh(GlMesh && r) : vertArray(r.vertArray), arrBuf(r.arrBuf), elemBuf(r.elemBuf), numVerts(r.numVerts), numElems(r.numElems) { r.vertArray=r.arrBuf=r.elemBuf=0; }
        ~GlMesh();

        void draw() const;

        void setElements(const uint32_t * elements, size_t numElements, GLenum usage = GL_STATIC_DRAW);
        void setVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage = GL_STATIC_DRAW);
        void setAttribute(GLuint loc, int size, GLenum type, bool normalized, size_t stride, const void * pointer);

                                 void setElements(const std::vector<uint32_t> & elements, GLenum usage = GL_STATIC_DRAW) { setElements(elements.data(), elements.size(), usage); }
        template<int N>          void setElements(const uint32_t (& elements)[N], GLenum usage = GL_STATIC_DRAW) { setElements(elements, N, usage); }
        template<class T>        void setVertices(const T * vertices, size_t numVertices) { setVertices(vertices, sizeof(T), numVertices); }
        template<class T, int N> void setVertices(const T (& vertices)[N]) { setVertices(vertices, N); }
        template<class T>        void setVertices(const std::vector<T> & vertices) { setVertices(vertices.data(), vertices.size()); }
        template<class T, int N> void setAttribute(int loc, const vec<float, N> T::*attribute) { setAttribute(loc, N, GL_FLOAT, false, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }
        template<class T, int N> void setAttribute(int loc, const vec<uint8_t, N> T::*attribute, bool normalized = true) { setAttribute(loc, N, GL_UNSIGNED_BYTE, normalized, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }

        GlMesh & operator = (GlMesh && r) { std::swap(vertArray, r.vertArray); std::swap(arrBuf, r.arrBuf); std::swap(elemBuf, r.elemBuf); numVerts=r.numVerts; numElems=r.numElems; return *this; }
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

        void use() const { glUseProgram(obj); }

        GlProgram & operator = (GlProgram && r) { std::swap(obj, r.obj); return *this; }
    };
}

#endif