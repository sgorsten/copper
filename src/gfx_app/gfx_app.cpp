#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>

int main(int argc, char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) return -1;
    auto win = SDL_CreateWindow("Graphics App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) return -1;
    auto glc = SDL_GL_CreateContext(win);

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
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.5f, -0.5f);
        glColor3f(0, 1, 0); glVertex2f( 0.0f, +0.5f);
        glColor3f(0, 0, 1); glVertex2f(+0.5f, -0.5f);
        glEnd();

        SDL_GL_SwapWindow(win);
    }

    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}