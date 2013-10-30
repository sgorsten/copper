#include "common.h"
#include "cu/plat.h"
#include <ostream>

using namespace cu;

Window::Window(const char * title, int2 dimensions) : Window()
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
}

Window::~Window()
{
    if (context) SDL_GL_DeleteContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::WriteGlVersion(std::ostream & out) const
{
    out << "OpenGL vendor:   " << glGetString(GL_VENDOR) << std::endl;
    out << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    out << "OpenGL version:  " << glGetString(GL_VERSION) << std::endl;
    out << "GLSL version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void Window::SwapBuffers()
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