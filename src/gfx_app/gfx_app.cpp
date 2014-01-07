#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

#include <iostream>
#include <string>
#include <vector>

class Material
{
    struct Parameter { std::string parameterName, uniformName; ptrdiff_t offset; GLint location; };
    std::string vertexShaderSource, fragmentShaderSource;
    std::vector<Parameter> parameters; size_t perObjectSize;

    GLuint vertexShader, fragmentShader, program;
    bool valid;

    void FreeObjects() 
    {
        if (program) glDeleteProgram(program);
        if (fragmentShader) glDeleteShader(fragmentShader);
        if (vertexShader) glDeleteShader(vertexShader);
        vertexShader = fragmentShader = program = 0;
    }
public:
    Material() : perObjectSize(), vertexShader(), fragmentShader(), program(), valid() {}
    ~Material() { FreeObjects(); }

    bool IsValid() const { return valid; }

    void SetVertexShaderSource(std::string source) { vertexShaderSource = move(source); valid = false; }
    void SetFragmentShaderSource(std::string source) { fragmentShaderSource = move(source); valid = false; }
    void AddPerObjectParameter(std::string name, std::string uniformName) { parameters.push_back({move(name), move(uniformName), 0, -1}); valid = false; }

    void Validate()
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
            perObjectSize += sizeof(float)*16;
        }

        valid = true;
    }

    void Use(const uint8_t * perObjectData) const
    {
        glUseProgram(program);
        for (auto & param : parameters) glUniform4fv(param.location, 1, reinterpret_cast<const float *>(perObjectData + param.offset));
    }

    size_t GetPerObjectSize() const { return perObjectSize; }
    void SetPerObjectParameter(uint8_t * perObjectData, const std::string & name, float x, float y, float z, float w)
    {
        for (auto & param : parameters)
        {
            if (param.parameterName == name)
            {
                reinterpret_cast<float *>(perObjectData + param.offset)[0] = x;
                reinterpret_cast<float *>(perObjectData + param.offset)[1] = y;
                reinterpret_cast<float *>(perObjectData + param.offset)[2] = z;
                reinterpret_cast<float *>(perObjectData + param.offset)[3] = w;
            }
        }
    }
};

int main(int argc, char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) return -1;
    auto win = SDL_CreateWindow("Graphics App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) return -1;
    auto glc = SDL_GL_CreateContext(win);

    if (glewInit() != GLEW_OK) return -1;

    Material m;
    m.SetVertexShaderSource(
        "#version 330\n"
        "in vec3 v_position;\n"
        "void main() { gl_Position = vec4(v_position,1); }"
    );
    m.SetFragmentShaderSource(
        "#version 330\n"
        "uniform vec4 u_color;\n"
        "out vec4 f_color;\n"
        "void main() { f_color = u_color; }"
    );
    m.AddPerObjectParameter("color", "u_color");

    try
    {
        m.Validate();
    }
    catch (const std::exception & e) 
    {
        std::cerr << e.what() << std::endl; 
        return -1;
    }

    std::vector<uint8_t> perObjectData;
    perObjectData.resize(m.GetPerObjectSize());
    m.SetPerObjectParameter(perObjectData.data(), "color", 1, 0.5f, 0, 1);

    bool quit = false;
    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        m.Use(perObjectData.data());

        glBegin(GL_TRIANGLES);
        glVertexAttrib3f(0, -0.5f, -0.5f, 0);
        glVertexAttrib3f(0,  0.0f, +0.5f, 0);
        glVertexAttrib3f(0, +0.5f, -0.5f, 0);
        glEnd();

        SDL_GL_SwapWindow(win);
    }

    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}