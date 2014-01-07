#include "tinydraw.h"

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
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) throw std::runtime_error("Invalid material - GLSL link error");

    perObjectSize = 0;
    for (auto & param : parameters)
    {
        auto loc = glGetUniformLocation(program, param.uniformName.c_str());
        if (loc == -1) throw std::runtime_error("Invalid material - Parameter has no matching uniform in shader");
        param.location = loc;
        param.offset = perObjectSize;
        perObjectSize += sizeof(float) * param.channels;
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