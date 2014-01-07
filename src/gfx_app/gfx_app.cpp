#include "tinydraw.h"

using namespace cu;

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
        "uniform vec3 u_translate;\n"
        "in vec3 v_position;\n"
        "void main() { gl_Position = vec4(v_position + u_translate, 1); }"
    );
    m.SetFragmentShaderSource(
        "#version 330\n"
        "uniform vec4 u_color;\n"
        "out vec4 f_color;\n"
        "void main() { f_color = u_color; }"
    );
    m.AddPerObjectParameter("translate", 3, "u_translate");
    m.AddPerObjectParameter("color", 4, "u_color");

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
    m.SetPerObjectParameter(perObjectData.data(), "translate", float3(0.3f, 0, 0));
    m.SetPerObjectParameter(perObjectData.data(), "color", float4(1, 0.5f, 0, 1));

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