// platform module - Contains utilities for interacting with the operating system (currently just convenient C++ wrappers around SDL types)
#ifndef COPPER_PLAT_H
#define COPPER_PLAT_H

#include "draw.h"

#include <SDL2/SDL.h>

namespace cu
{
    class Window
    {
        SDL_Window *    window;
        SDL_GLContext   context;
        Window() : window(), context() {}
    public:
        Window(const char * title, int2 dimensions);
        ~Window();

        void WriteGlVersion(std::ostream & out) const;
        void SwapBuffers();
    };
}

#endif