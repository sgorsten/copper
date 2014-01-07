#include "tinydraw.h"

using namespace cu;

void RenderScene(const DrawList & drawList)
{
    for (auto & obj : drawList.objects)
    {
        obj.material->Use(drawList.objectData.data() + obj.paramOffset);
        obj.mesh->DrawQuads();
    }
}

int main(int argc, char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) return -1;
    auto win = SDL_CreateWindow("Graphics App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) return -1;
    auto glc = SDL_GL_CreateContext(win);

    if (glewInit() != GLEW_OK) return -1;

    // Create a material

    Material mat;
    mat.SetVertexShaderSource(R"(#version 330
        uniform vec3 u_translate;
        uniform vec4 u_color;

        in vec3 v_position;
        in vec3 v_color;
        out vec3 color;

        void main() 
        { 
            color = v_color * u_color.rgb;
            gl_Position = vec4(v_position + u_translate, 1);
        })"
    );
    mat.SetFragmentShaderSource(R"(#version 330
        in vec3 color;
        out vec4 f_color;
        void main()
        {
            f_color = vec4(color,1); 
        })"
    );
    mat.AddVertexAttribute(0, "v_position");
    mat.AddVertexAttribute(1, "v_color");
    mat.AddPerObjectParameter("translate", 3, "u_translate");
    mat.AddPerObjectParameter("color", 4, "u_color");
    try
    {
        mat.Validate();
    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    // Create a mesh

    struct ColorVertex { float3 position,color; };
    const std::vector<ColorVertex> verts = {
        { { -0.5f, -0.5f, 0 }, { 1, 0, 0 } },
        { { +0.5f, -0.5f, 0 }, { 1, 1, 0 } },
        { { +0.5f, +0.5f, 0 }, { 0, 1, 0 } },
        { { -0.5f, +0.5f, 0 }, { 0, 0, 1 } },
    };

    Mesh mesh;
    mesh.SetVertices(verts);
    mesh.SetAttribute(0, &ColorVertex::position);
    mesh.SetAttribute(1, &ColorVertex::color);

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

        DrawList drawList;

        drawList.AddObject(&mat, &mesh);
        drawList.SetParam("translate", float3(0.3f, 0, 0));
        drawList.SetParam("color", float4(1, 0.5f, 0, 1));

        drawList.AddObject(&mat, &mesh);
        drawList.SetParam("translate", float3(-0.2f, -0.2f, 0));
        drawList.SetParam("color", float4(0, 0, 1, 1));

        drawList.AddObject(&mat, &mesh);
        drawList.SetParam("color", float4(0, 1, 0, 1));
        // Note that although we do not specify translate, there is no "bleeding" of state from the previous object

        glClear(GL_COLOR_BUFFER_BIT);
        RenderScene(drawList);
        SDL_GL_SwapWindow(win);
    }

    SDL_GL_DeleteContext(glc);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}