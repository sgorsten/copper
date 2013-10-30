#include "common.h"
#include "cu/draw.h"
#include <cassert>

using namespace cu;

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
}