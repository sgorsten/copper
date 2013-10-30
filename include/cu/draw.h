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
        GLenum mode, itype;

        GlMesh(const GlMesh & r) = delete;
        GlMesh & operator = (const GlMesh & r) = delete;

        void setAttribList(int loc) {}
        template<class T, class... R> void setAttribList(int loc, T attrib, R... attribs) { setAttribute(loc, attrib); setAttribList(loc+1, attribs...); }
    public:
        GlMesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems(), mode(GL_TRIANGLES), itype() {}
        GlMesh(GlMesh && r) : vertArray(r.vertArray), arrBuf(r.arrBuf), elemBuf(r.elemBuf), numVerts(r.numVerts), numElems(r.numElems), mode(r.mode), itype(r.itype) { r.vertArray=r.arrBuf=r.elemBuf=0; }
        template<class V, class E, class... R> GlMesh(const std::vector<V> & verts, const std::vector<E> & elems, R... attribs) : GlMesh() { setVertices(verts); setElements(elems); setAttribList(0, attribs...); }
        ~GlMesh();

        void draw() const;

        void setElements(const void * elements, size_t indexSize, size_t elemSize, size_t numElements, GLenum usage);
        void setVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage);
        void setAttribute(int loc, int size, GLenum type, bool normalized, size_t stride, const void * pointer);
        void setAttribute(int loc, std::nullptr_t);

        template<class T, int N> void setElements(const std::vector<vec<T,N>> & elems, GLenum usage = GL_STATIC_DRAW) { setElements(&elems[0].x, sizeof(T), N, elems.size(), usage); }
        template<class T>        void setVertices(const std::vector<T> & vertices, GLenum usage = GL_STATIC_DRAW) { setVertices(vertices.data(), sizeof(T), vertices.size(), usage); }
        template<class T, int N> void setAttribute(int loc, const vec<float, N> T::*attribute) { setAttribute(loc, N, GL_FLOAT, false, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }
        template<class T, int N> void setAttribute(int loc, const vec<uint8_t, N> T::*attribute, bool normalized = true) { setAttribute(loc, N, GL_UNSIGNED_BYTE, normalized, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }

        GlMesh & operator = (GlMesh && r) { std::swap(vertArray, r.vertArray); std::swap(arrBuf, r.arrBuf); std::swap(elemBuf, r.elemBuf); numVerts=r.numVerts; numElems=r.numElems; mode=r.mode; itype=r.itype; return *this; }
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
        void uniform(const char * name, const float4x4 & mat) const { glUniformMatrix4fv(glGetUniformLocation(obj, name), 1, GL_FALSE, &mat.x.x); }
        void uniform(const char * name, const float3 & vec) const { glUniform3fv(glGetUniformLocation(obj, name), 1, &vec.x); }

        GlProgram & operator = (GlProgram && r) { std::swap(obj, r.obj); return *this; }
    };
}

#endif