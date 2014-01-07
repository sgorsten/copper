#include "tinydraw.h"

void Mesh::DrawQuads() const
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glDrawArrays(GL_QUADS, 0, 4);
}

void Mesh::SetVertices(const void * vertices, size_t stride, size_t count)
{
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, stride*count, vertices, GL_STATIC_DRAW);
    vertexStride = stride;
    vertexCount = count;
}

void Mesh::SetAttribute(int index, size_t channels, const void * pointer)
{
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, channels, GL_FLOAT, GL_FALSE, vertexStride, pointer);
}

void Material::FreeObjects()
{
    if (program) glDeleteProgram(program);
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (vertexShader) glDeleteShader(vertexShader);
    vertexShader = fragmentShader = program = 0;
}

void Material::Validate()
{
    FreeObjects();

    GLint status;

    if (!vertexShader) vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto source = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &source, 0);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) throw std::runtime_error("Invalid material - GLSL compile error");

    if (!fragmentShader) fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    source = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &source, 0);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) throw std::runtime_error("Invalid material - GLSL compile error");

    if (!program) program = glCreateProgram();
    for (auto & attrib : attributes) glBindAttribLocation(program, attrib.index, attrib.attributeName.c_str());
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) throw std::runtime_error("Invalid material - GLSL link error");

    // Validate that all requested attributes are present in shader
    for (auto & attrib : attributes)
    {
        if(glGetAttribLocation(program, attrib.attributeName.c_str()) != attrib.index) throw std::runtime_error("Invalid material - Missing program attribute: " + attrib.attributeName);
    }

    // Validate that all requested parameters have an associated uniform in shader
    for (auto & param : parameters)
    {
        param.location = glGetUniformLocation(program, param.uniformName.c_str());
        if (param.location == -1) throw std::runtime_error("Invalid material - Missing program uniform: " + param.uniformName);
    }

    valid = true;
}

void Material::Use(const uint8_t * perObjectData) const
{
    glUseProgram(program);
    for (auto & param : parameters)
    {
        switch (param.channels)
        {
        case 1: glUniform1fv(param.location, 1, reinterpret_cast<const float *>(perObjectData + param.offset)); break;
        case 2: glUniform2fv(param.location, 1, reinterpret_cast<const float *>(perObjectData + param.offset)); break;
        case 3: glUniform3fv(param.location, 1, reinterpret_cast<const float *>(perObjectData + param.offset)); break;
        case 4: glUniform4fv(param.location, 1, reinterpret_cast<const float *>(perObjectData + param.offset)); break;
        }
    }
}