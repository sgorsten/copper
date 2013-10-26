#include <cu/math.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

#include <iostream>

using namespace cu;

struct ColorVertex
{
    float3 position;
    float3 color;
};

class Mesh
{
    GLuint vertArray,arrBuf,elemBuf;
    GLsizei numVerts,numElems;
public:
    Mesh() : vertArray(), arrBuf(), elemBuf(), numVerts(), numElems() {}

    void SetVertices(const void * vertices, size_t vertexSize, size_t numVertices, GLenum usage = GL_STATIC_DRAW)
    {
        if (!arrBuf) glGenBuffers(1, &arrBuf);
        glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
        glBufferData(GL_ARRAY_BUFFER, vertexSize*numVertices, vertices, usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        numVerts = numVertices;
    }

    void SetAttribute(GLuint loc, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
    {
        if (!vertArray) glGenVertexArrays(1, &vertArray);
        glBindVertexArray(vertArray);

        if (!arrBuf) glGenBuffers(1, &arrBuf);
        glBindBuffer(GL_ARRAY_BUFFER, arrBuf);
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, size, type, normalized, stride, pointer);

        glBindVertexArray(0);
    }

    void SetIndices(const uint32_t * indices, size_t numIndices, GLenum usage = GL_STATIC_DRAW)
    {
        if (!vertArray) glGenVertexArrays(1, &vertArray);
        glBindVertexArray(vertArray);

        if (!elemBuf) glGenBuffers(1, &elemBuf);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBuf);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*numIndices, indices, usage);
        numElems = numIndices;

        glBindVertexArray(0);
    }

    template<class T> void SetVertices(const T * vertices, size_t numVertices) { SetVertices(vertices, sizeof(T), numVertices); }
    template<class T, int N> void SetAttribute(int loc, const vec<float,N> T::*attribute) { SetAttribute(loc, N, GL_FLOAT, GL_FALSE, sizeof(T), &(reinterpret_cast<const T *>(0)->*attribute)); }

    void Draw() const
    {
        if (!vertArray || numVerts == 0) return;
        glBindVertexArray(vertArray);
        if(numElems > 0) glDrawElements(GL_TRIANGLES, numElems, GL_UNSIGNED_INT, 0);
        else glDrawArrays(GL_TRIANGLES, 0, numVerts);
    }
};

int main(int argc, char * argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1) return -1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto win = SDL_CreateWindow("Graphics App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) return -1;
    auto glc = SDL_GL_CreateContext(win);
    if (!glc) return -1;

    std::cout << "OpenGL vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    if (glewInit() != GLEW_OK) return -1;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char * vsText = R"(
        #version 330
        layout(location = 0) in vec4 v_position;
        layout(location = 1) in vec3 v_color;
        out vec3 color;
        void main()
        {
            gl_Position = v_position;
            color = v_color;
        }
    )";
    glShaderSource(vs, 1, &vsText, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char * fsText = R"(
        #version 330
        in vec3 color;
        layout(location = 0) out vec4 f_color;
        void main()
        {
            f_color = vec4(color,1);
        }
    )";
    glShaderSource(fs, 1, &fsText, nullptr);
    glCompileShader(fs);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    const ColorVertex verts[] = {
        {{ -0.5f, -0.5f, 0 }, { 1, 0, 0 }},
        {{ +0.5f, -0.5f, 0 }, { 1, 1, 0 }},
        {{ +0.5f, +0.5f, 0 }, { 0, 1, 0 }},
        {{ -0.5f, +0.5f, 0 }, { 0, 0, 1 } },
    };
    const uint32_t indices[] = {0,1,2, 0,2,3};
    Mesh m;
    m.SetVertices(verts, 4);
    m.SetAttribute(0, &ColorVertex::position);
    m.SetAttribute(1, &ColorVertex::color);
    m.SetIndices(indices, 6);

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

        glUseProgram(prog);
        m.Draw();

        SDL_GL_SwapWindow(win);
    }

    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}