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
        class Mesh
        {
            GLuint vertArray, arrBuf, elemBuf;
            GLsizei numVerts, numElems;

            Mesh(const Mesh & r); Mesh & operator = (const Mesh & r); // No copy
        public:
            Mesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems() {}
            Mesh(Mesh && r) : vertArray(r.vertArray), arrBuf(r.arrBuf), elemBuf(r.elemBuf), numVerts(r.numVerts), numElems(r.numElems) { r.vertArray=r.arrBuf=r.elemBuf=0; }
            ~Mesh();

            void Draw() const;

            void SetElements(const uint32_t * elements, size_t numElements, GLenum usage = GL_STATIC_DRAW);
            void SetVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage = GL_STATIC_DRAW);
            void SetAttribute(GLuint loc, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);

            template<class T> void SetVertices(const T * vertices, size_t numVertices) { SetVertices(vertices, sizeof(T), numVertices); }
            template<class T> void SetVertices(const std::vector<T> & vertices) { SetVertices(vertices.data(), vertices.size()); }
            template<class T, int N> void SetAttribute(int loc, const vec<float, N> T::*attribute) { SetAttribute(loc, N, GL_FLOAT, GL_FALSE, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }

            Mesh & operator = (Mesh && r) { std::swap(vertArray, r.vertArray); std::swap(arrBuf, r.arrBuf); std::swap(elemBuf, r.elemBuf); numVerts=r.numVerts; numElems=r.numElems; return *this; }
        };
    }
}

#endif