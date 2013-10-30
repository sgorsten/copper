#include <cu/geom.h>
#include <cu/mesh.h>
#include <cu/plat.h>
#include <cu/refl.h>

#include <vector>
#include <iostream>

using namespace cu;

struct BasicVertex { float3 pos, norm; float2 uv; };
struct ColorVertex { float3 pos; ubyte4 col; };

#define FIELD(n) f(#n, o.n)
template<class F> void visit_fields(BasicVertex & o, F f) { FIELD(pos); FIELD(norm); FIELD(uv); }
template<class F> void visit_fields(ColorVertex & o, F f) { FIELD(pos); FIELD(col); }
template<class V, class I, class F> void visit_fields(TriMesh<V,I> & o, F f) { FIELD(verts); FIELD(tris); }
#undef FIELD

TriMesh<BasicVertex,uint8_t> basicBox(const float3 & dims) { return boxMesh(dims, &BasicVertex::pos, &BasicVertex::norm, &BasicVertex::uv); }
TriMesh<ColorVertex,uint8_t> colorBox(const float3 & dims, const ubyte4 & col)
{
    auto mesh = boxMesh(dims, &ColorVertex::pos);
    setField(mesh.verts, &ColorVertex::col, col);
    return mesh;
}

template<class I> GlMesh glMesh(const TriMesh<BasicVertex,I> & mesh) { return GlMesh(mesh.verts, mesh.tris, &BasicVertex::pos, &BasicVertex::norm, &BasicVertex::uv); }
template<class I> GlMesh glMesh(const TriMesh<ColorVertex,I> & mesh) { return GlMesh(mesh.verts, mesh.tris, &ColorVertex::pos, &ColorVertex::col); }

struct Object
{
    GlProgram * prog;
    GlMesh      mesh;
    Pose        pose;
};

int main(int argc, char * argv[])
{
    try
    {
        Window window("Graphics App", {640,480});
        window.WriteGlVersion(std::cout);

        GlProgram unlitProg = {
            {GL_VERTEX_SHADER, R"(
                #version 330
                uniform mat4 u_matClipFromModel;
                layout(location = 0) in vec3 v_position;
                layout(location = 1) in vec4 v_color;
                out vec4 color;
                void main()
                {
                    gl_Position = u_matClipFromModel * vec4(v_position,1);
                    color       = v_color;
                }
            )"},
            {GL_FRAGMENT_SHADER, R"(
                #version 330
                in vec4 color;
                layout(location = 0) out vec4 f_color;
                void main()
                {
                    f_color = color;
                }
            )"}
        };

        GlProgram litProg = {
            {GL_VERTEX_SHADER, R"(
                #version 330
                uniform mat4 u_matClipFromModel;
                uniform mat4 u_matViewFromModel;
                layout(location = 0) in vec3 v_position;
                layout(location = 1) in vec3 v_normal;
                layout(location = 2) in vec2 v_texCoord;
                out vec3 position;
                out vec3 normal;
                out vec2 texCoord;
                void main()
                {
                    gl_Position = u_matClipFromModel * vec4(v_position,1);
                    position    = (u_matViewFromModel * vec4(v_position,1)).xyz; // Assume transform outputs w=1
                    normal      = (u_matViewFromModel * vec4(v_normal,0)).xyz;
                    texCoord    = v_texCoord;
                }
            )"},
            {GL_FRAGMENT_SHADER, R"(
                #version 330
                uniform vec3 u_lightPos;
                in vec3 position;
                in vec3 normal;
                in vec2 texCoord;
                layout(location = 0) out vec4 f_color;
                void main()
                {
                    vec3 lightDir = normalize(u_lightPos - position);
                    vec3 eyeDir   = normalize(-position);
                    vec3 halfDir  = normalize(lightDir + eyeDir);
                    vec3 n        = normalize(normal);

                    vec3 light = vec3(0.2,0.2,0.2);
                    light += vec3(0.8,0.8,0.8) * max(dot(lightDir, n), 0);
                    light += vec3(1,1,1) * pow(max(dot(halfDir, n), 0), 256);
                    f_color = vec4(abs(light), 1);
                }
            )"}
        };

        // Define scene
        Object objs[] = {
            {&litProg,   glMesh(basicBox({1.2f, 1.0f, 0.8f})),                       float3(0, 0,-4)},
            {&unlitProg, glMesh(colorBox({0.2f, 0.2f, 0.2f}, {255, 255, 127, 255})), float3(0, 4,-8)},
            {&litProg,   glMesh(basicBox(float3(8, 1, 8))),                          float3(0,-2,-4)}
        };

        float t=0;
        Pose camPose;

        bool quit = false;
        bool w={}, s={}, a={}, d={}, ml={};
        float yaw=0, pitch=0;
        auto t0 = SDL_GetTicks();
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
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    switch (e.key.keysym.sym)
                    {
                    case SDLK_w: w = e.key.state == SDL_PRESSED; break;
                    case SDLK_s: s = e.key.state == SDL_PRESSED; break;
                    case SDLK_a: a = e.key.state == SDL_PRESSED; break;
                    case SDLK_d: d = e.key.state == SDL_PRESSED; break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_RIGHT) ml = e.button.state == SDL_PRESSED;
                    break;
                case SDL_MOUSEMOTION:
                    if (ml)
                    {
                        yaw -= e.motion.xrel * 0.01f;
                        pitch -= e.motion.yrel * 0.01f;
                    }
                    break;
                }
            }
            auto t1 = SDL_GetTicks();
            auto timestep = (t1-t0)*0.001f;
            t0 = t1;

            t += timestep*2;

            float3 move;
            if(w) move.z -= 1;
            if(a) move.x -= 1;
            if(s) move.z += 1;
            if(d) move.x += 1;
            camPose.orientation = qmul(qrotation(float3(0,1,0), yaw), qrotation(float3(1,0,0), pitch));
            camPose.position = camPose * (safenorm(move)*(timestep*10));
            objs[0].pose.orientation = qrotation(float3(0,1,0), t);

            auto clipFromView   = perspectiveMatrixRhGl(1.0f, 1.333f, 0.1f, 64.0f);
            auto viewFromWorld   = camPose.inverse().matrix();

            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            for (auto & obj : objs)
            {
                auto viewFromModel = mul(viewFromWorld, obj.pose.matrix());

                obj.prog->use();
                obj.prog->uniform("u_matClipFromModel", mul(clipFromView, viewFromModel));
                obj.prog->uniform("u_matViewFromModel", viewFromModel);
                obj.prog->uniform("u_lightPos", transformCoord(viewFromWorld, objs[1].pose.position));
                obj.mesh.draw();
            }
            
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
