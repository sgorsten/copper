#include <cu/draw.h>

#include <SDL.h>

#include <vector>
#include <iostream>

using namespace cu;

struct ColorVertex
{
    float3 position;
    ubyte4 color;
};

GlMesh CreateMesh(const std::vector<ColorVertex> & verts)
{
    GlMesh m;
    m.setVertices(verts);
    m.setAttribute(0, &ColorVertex::position);
    m.setAttribute(1, &ColorVertex::color);
    return m;
}



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

    GlProgram prog = {
        {GL_VERTEX_SHADER, R"(
            #version 330
            layout(location = 0) in vec4 v_position;
            layout(location = 1) in vec3 v_color;
            out vec3 color;
            void main()
            {
                gl_Position = v_position;
                color = v_color;
            }
        )"},
        {GL_FRAGMENT_SHADER, R"(
            #version 330
            in vec3 color;
            layout(location = 0) out vec4 f_color;
            void main()
            {
                f_color = vec4(color,1);
            }
        )"}
    };

    auto mesh = CreateMesh({
        { { -0.5f, -0.5f, 0 }, { 255,   0,   0, 255 } },
        { { +0.5f, -0.5f, 0 }, { 255, 255,   0, 255 } },
        { { +0.5f, +0.5f, 0 }, {   0, 255,   0, 255 } },
        { { -0.5f, +0.5f, 0 }, {   0,   0, 255, 255 } },
    });
    const uint32_t elems[] = {0,1,2, 0,2,3};
    mesh.setElements(elems, 6);

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

        prog.use();
        mesh.draw();

        SDL_GL_SwapWindow(win);
    }

    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}