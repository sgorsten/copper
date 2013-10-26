#ifndef COPPER_DRAW_H
#define COPPER_DRAW_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <vector>

#include "math.h"

namespace cu
{
    namespace gl
    {
        class mesh
        {
            GLuint vertArray, arrBuf, elemBuf;
            GLsizei numVerts, numElems;

            mesh(const mesh & r); mesh & operator = (const mesh & r); // No copy
        public:
            mesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems() {}
            mesh(mesh && r) : vertArray(r.vertArray), arrBuf(r.arrBuf), elemBuf(r.elemBuf), numVerts(r.numVerts), numElems(r.numElems) { r.vertArray=r.arrBuf=r.elemBuf=0; }
            ~mesh();

            void Draw() const;

            void SetElements(const uint32_t * elements, size_t numElements, GLenum usage = GL_STATIC_DRAW);
            void SetVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage = GL_STATIC_DRAW);
            void SetAttribute(GLuint loc, int size, GLenum type, bool normalized, size_t stride, const void * pointer);

            template<class T> void SetVertices(const T * vertices, size_t numVertices) { SetVertices(vertices, sizeof(T), numVertices); }
            template<class T> void SetVertices(const std::vector<T> & vertices) { SetVertices(vertices.data(), vertices.size()); }
            template<class T, int N> void SetAttribute(int loc, const vec<float, N> T::*attribute) { SetAttribute(loc, N, GL_FLOAT, false, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }
            template<class T, int N> void SetAttribute(int loc, const vec<uint8_t, N> T::*attribute, bool normalized = true) { SetAttribute(loc, N, GL_UNSIGNED_BYTE, normalized, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }

            mesh & operator = (mesh && r) { std::swap(vertArray, r.vertArray); std::swap(arrBuf, r.arrBuf); std::swap(elemBuf, r.elemBuf); numVerts=r.numVerts; numElems=r.numElems; return *this; }
        };

        class shader
        {
            friend class program;
            GLuint obj;

            shader(const shader & r); shader & operator = (const shader & r); // No copy
        public:
            shader() : obj() {}
            shader(GLenum type, const char * source);
            shader(shader && r) : obj(r.obj) { r.obj = 0; }
            ~shader() { glDeleteShader(obj); }

            const GLuint _obj() const { return obj; }

            shader & operator = (shader && r) { std::swap(obj, r.obj); return *this; }
        };

        class program
        {
            GLuint obj;
            shader vs,fs;

            program(const program & r); program & operator = (const program & r); // No copy
        public:
            program() : obj() {}
            program(shader vs, shader fs);
            program(program && r) : obj(r.obj) { r.obj = 0; }
            ~program() { glDeleteProgram(obj); }

            void Use() const { glUseProgram(obj); }

            program & operator = (program && r) { std::swap(obj, r.obj); return *this; }
        };
    }
}

#endif