#include <cu/geom.h>
#include <cu/mesh.h>
#include <cu/plat.h>
#include <cu/refl.h>

#include <iostream>

using namespace cu;

// Define and reflect a simple vertex type
struct ColorVertex { float2 pos; float3 col; };
template<class F> void visit_fields(ColorVertex & o, F f) { f("pos",o.pos); f("col",o.col); }

int main(int argc, char * argv[])
{
    try
    {
        Window window("Basic App", { 512, 512 });
        window.WriteGlVersion(std::cout);

        // Load a simple shader that interpolates vertex colors
        GlProgram prog = {
            { GL_VERTEX_SHADER, {R"(
                #version 330
                uniform Transform { mat4 transform; };
                layout(location = 0) in vec4 v_position;
                layout(location = 1) in vec4 v_color;
                out vec4 color;
                void main()
                {
                    gl_Position = transform * v_position;
                    color       = v_color;
                }
            )"}},
            { GL_FRAGMENT_SHADER, {R"(
                #version 330
                in vec4 color;
                layout(location = 0) out vec4 f_color;
                void main()
                {
                    f_color = color;
                }
            )"}}
        };

        auto block = prog.block("Transform");
        std::vector<uint8_t> buffer(block->pack.size);
        GlUniformBuffer ubo(block->pack.size, GL_DYNAMIC_DRAW);
        ubo.bind(block->binding);

        // Load a triangle mesh from a JSON-based format
        auto triMesh = decodeJson<TriMesh<ColorVertex, uint16_t>>(R"(
        {
            "verts":[
                {"pos":[0,0], "col":[0.7,0.7,0.7]},
                {"pos":[-0.86,-0.5], "col":[1,0,0]},
                {"pos":[ 0.00,-1.0], "col":[1,1,0]},
                {"pos":[ 0.86,-0.5], "col":[0,1,0]},
                {"pos":[ 0.86, 0.5], "col":[0,1,1]},
                {"pos":[ 0.00, 1.0], "col":[0,0,1]},
                {"pos":[-0.86, 0.5], "col":[1,0,1]}
            ],
            "tris":[[0,1,2],[0,2,3],[0,3,4],[0,4,5],[0,5,6],[0,6,1]]
        })");

        // Create a renderable mesh, using .pos as attrib 0 and .col as attrib 1
        auto mesh = GlMesh(triMesh.verts, triMesh.tris, &ColorVertex::pos, &ColorVertex::col);

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

            Pose pose = qrotation(float3(0, 0, 1), SDL_GetTicks()*0.003f);

            block->set(buffer, "transform", pose.matrix());
            ubo.setData(buffer, GL_DYNAMIC_DRAW);

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
