#include "cu/draw.h"

using namespace cu::gl;

mesh::~mesh()
{
    glDeleteVertexArrays(1,&vertArray);
    glDeleteBuffers(1,&elemBuf);
    glDeleteBuffers(1,&arrBuf);
}

void mesh::SetElements(const uint32_t * elements, size_t numElements, GLenum usage)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!elemBuf) glGenBuffers(1, &elemBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*numElements, elements, usage);
    numElems = numElements;

    glBindVertexArray(0);
}

void mesh::SetVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage)
{
    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*numVertices, vertices, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    numVerts = numVertices;
}

void mesh::SetAttribute(GLuint loc, int size, GLenum type, bool normalized, size_t stride, const void * pointer)
{
    if (!vertArray) glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);

    if (!arrBuf) glGenBuffers(1, &arrBuf);
    glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);

    glBindVertexArray(0);
}

void mesh::Draw() const
{
    if (!vertArray || numVerts == 0) return;
    glBindVertexArray(vertArray);
    if (numElems > 0) glDrawElements(GL_TRIANGLES, numElems, GL_UNSIGNED_INT, 0);
    else glDrawArrays(GL_TRIANGLES, 0, numVerts);
}

shader::shader(GLenum type, const char * source) : shader()
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

program::program(shader _vs, shader _fs) : program()
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