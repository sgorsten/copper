#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

#include <iostream>
#include <string>
#include <vector>

#include <cu/math.h>

class Mesh
{
    GLuint vertexBuffer, vertexStride, vertexCount;
public:
    Mesh() : vertexBuffer(), vertexStride(), vertexCount() {}

    void SetVertices(const void * vertices, size_t stride, size_t count);
    void SetAttribute(int index, size_t channels, const void * pointer);
    template<class T> void SetVertices(const std::vector<T> & vertices) { SetVertices(vertices.data(), sizeof(T), vertices.size()); }
    template<class T, int N> void SetAttribute(int index, const cu::vec<float,N> T::*field) { SetAttribute(index, N, &(reinterpret_cast<const T *>(0)->*field)); }

    void DrawQuads() const;
};

class Material
{
    struct Attribute { size_t index; std::string attributeName; };
    struct Parameter { std::string parameterName, uniformName; size_t channels; ptrdiff_t offset; GLint location; 
        void Set(uint8_t * data, size_t index, float value) const { if (index < channels) reinterpret_cast<float *>(data)[index] = value; }
    };
    std::string vertexShaderSource, fragmentShaderSource;
    std::vector<Parameter> parameters; size_t perObjectSize;
    std::vector<Attribute> attributes;

    GLuint vertexShader, fragmentShader, program;
    bool valid;

    void FreeObjects();
public:
    Material() : perObjectSize(), vertexShader(), fragmentShader(), program(), valid() {}
    ~Material() { FreeObjects(); }

    bool IsValid() const { return valid; }

    void SetVertexShaderSource(std::string source) { vertexShaderSource = move(source); valid = false; }
    void SetFragmentShaderSource(std::string source) { fragmentShaderSource = move(source); valid = false; }
    void AddVertexAttribute(size_t index, std::string attributeName) { attributes.push_back({index, attributeName}); valid = false; }
    void AddPerObjectParameter(std::string name, size_t channels, std::string uniformName) { parameters.push_back({ move(name), move(uniformName), channels, perObjectSize, -1 }); perObjectSize += sizeof(float) * channels; valid = false; }
    void Validate();

    void Use(const uint8_t * perObjectData) const;

    size_t GetPerObjectSize() const { return perObjectSize; }
    template<int N> void SetPerObjectParameter(uint8_t * perObjectData, const std::string & name, const cu::vec<float,N> & v) const { for (auto & param : parameters) if (param.parameterName == name) for (int i = 0; i<N; ++i) param.Set(perObjectData + param.offset, i, v[i]); }
};

struct DrawList
{
    struct Object { const Material * material; const Mesh * mesh; size_t paramOffset; };
    std::vector<Object> objects;
    std::vector<uint8_t> objectData;

    void AddObject(const Material * material, const Mesh  * mesh) { objects.push_back({ material, mesh, objectData.size() }); objectData.resize(objectData.size() + material->GetPerObjectSize()); }
    template<class T> void SetParam(const std::string & name, const T & value) { objects.back().material->SetPerObjectParameter(objectData.data() + objects.back().paramOffset, name, value); }
};