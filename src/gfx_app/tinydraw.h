#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

#include <iostream>
#include <string>
#include <vector>

#include <cu/math.h>

class Material
{
    struct Parameter { std::string parameterName, uniformName; size_t channels; ptrdiff_t offset; GLint location; 
        void Set(uint8_t * data, size_t index, float value) const { if (index < channels) reinterpret_cast<float *>(data)[index] = value; }
    };
    std::string vertexShaderSource, fragmentShaderSource;
    std::vector<Parameter> parameters; size_t perObjectSize;

    GLuint vertexShader, fragmentShader, program;
    bool valid;

    void FreeObjects();
public:
    Material() : perObjectSize(), vertexShader(), fragmentShader(), program(), valid() {}
    ~Material() { FreeObjects(); }

    bool IsValid() const { return valid; }

    void SetVertexShaderSource(std::string source) { vertexShaderSource = move(source); valid = false; }
    void SetFragmentShaderSource(std::string source) { fragmentShaderSource = move(source); valid = false; }
    void AddPerObjectParameter(std::string name, size_t channels, std::string uniformName) { parameters.push_back({ move(name), move(uniformName), channels, 0, -1 }); valid = false; }

    void Validate();

    void Use(const uint8_t * perObjectData) const;

    size_t GetPerObjectSize() const { return perObjectSize; }
    template<int N> void SetPerObjectParameter(uint8_t * perObjectData, const std::string & name, const cu::vec<float,N> & v) { for (auto & param : parameters) if (param.parameterName == name) for (int i = 0; i<N; ++i) param.Set(perObjectData + param.offset, i, v[i]); }
};