#include <cu/draw.h>
#include <cu/refl.h>

#include <SDL2/SDL.h>

#include <vector>
#include <iostream>

using namespace cu;

class Window
{
    SDL_Window *    window;
    SDL_GLContext   context;
    Window() : window(), context() {}
public:
    Window(const char * title, int2 dimensions) : Window()
    {
        if (SDL_Init(SDL_INIT_VIDEO) == -1) throw std::runtime_error("SDL_Init() failed");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dimensions.x, dimensions.y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if (!window) throw std::runtime_error("SDL_CreateWindow() failed");
        context = SDL_GL_CreateContext(window);
        if (!context) throw std::runtime_error("SDL_GL_CreateContext() failed");
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) throw std::runtime_error("glewInit() failed");
        glGetError(); // Often returns GL_INVALID_ENUM after calling glewInit()

        std::cout << "OpenGL vendor:   " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "OpenGL version:  " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    }

    ~Window()
    {
        if(context) SDL_GL_DeleteContext(context);
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SwapBuffers()
    {
        #ifdef DEBUG_GL
        switch (glGetError())
        {
        case GL_INVALID_ENUM: throw std::runtime_error("GL_INVALID_ENUM");
        case GL_INVALID_VALUE: throw std::runtime_error("GL_INVALID_VALUE");
        case GL_INVALID_OPERATION: throw std::runtime_error("GL_INVALID_OPERATION");
        case GL_INVALID_FRAMEBUFFER_OPERATION: throw std::runtime_error("GL_INVALID_FRAMEBUFFER_OPERATION");
        case GL_OUT_OF_MEMORY: throw std::runtime_error("GL_OUT_OF_MEMORY");
        case GL_STACK_UNDERFLOW: throw std::runtime_error("GL_STACK_UNDERFLOW");
        case GL_STACK_OVERFLOW: throw std::runtime_error("GL_STACK_OVERFLOW");
        }
        #endif

        SDL_GL_SwapWindow(window);
    }
};

struct ColorVertex { float2 pos; float3 col; };
struct ColorMesh { std::vector<ColorVertex> verts; std::vector<uint32_t> tris; };

#define FIELD(n) f(#n, o.n)
template<class F> void visit_fields(ColorVertex & o, F f) { FIELD(pos); FIELD(col); }
template<class F> void visit_fields(ColorMesh & o, F f) { FIELD(verts); FIELD(tris); }
#undef FIELD

int main(int argc, char * argv[])
{
    try
    {
        Window window("Graphics App", {640,480});

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

        auto meshData = decodeJson<ColorMesh>(R"(
        {
            "verts":[
                {"pos":[-0.5,-0.3], "col":[1,0,0]},
                {"pos":[ 0.0,-0.7], "col":[1,1,0]},
                {"pos":[ 0.5,-0.3], "col":[0,1,0]},
                {"pos":[ 0.5, 0.3], "col":[0,1,1]},
                {"pos":[ 0.0, 0.7], "col":[0,0,1]},
                {"pos":[-0.5, 0.3], "col":[1,0,1]}
            ],
            "tris":[0,1,2, 0,2,3, 0,3,4, 0,4,5]
        })");

        GlMesh mesh;
        mesh.setAttribute(0, &ColorVertex::pos);
        mesh.setAttribute(1, &ColorVertex::col);
        mesh.setVertices(meshData.verts);
        mesh.setElements(meshData.tris);

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
            
            window.SwapBuffers();
        }
        return 0;
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        return -1;
    }
}
