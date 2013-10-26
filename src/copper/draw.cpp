#include "cu/draw.h"

using namespace cu::gl;

Mesh::~Mesh()
{
    glDeleteVertexArrays(1,&vertArray);
    glDeleteBuffers(1,&elemBuf);
    glDeleteBuffers(1,&arrBuf);
}

void Mesh::SetElements(const uint32_t * elements, size_t numElements, GLenum usage)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!elemBuf) glGenBuffers(1, &elemBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*numElements, elements, usage);
    numElems = numElements;

    glBindVertexArray(0);
}

void Mesh::SetVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage)
{
    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*numVertices, vertices, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    numVerts = numVertices;
}

void Mesh::SetAttribute(GLuint loc, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, size, type, normalized, stride, pointer);

    glBindVertexArray(0);
}

void Mesh::Draw() const
{
    if (!vertArray || numVerts == 0) return;
    glBindVertexArray(vertArray);
    if (numElems > 0) glDrawElements(GL_TRIANGLES, numElems, GL_UNSIGNED_INT, 0);
    else glDrawArrays(GL_TRIANGLES, 0, numVerts);
}