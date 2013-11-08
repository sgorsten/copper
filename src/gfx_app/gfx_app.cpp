#include <cu/geom.h>
#include <cu/mesh.h>
#include <cu/plat.h>
#include <cu/refl.h>
#include <cu/load.h>

#include <vector>
#include <iostream>
#include <memory>

#include "render.h"

template<class T> std::shared_ptr<T> shared(T && r) { return std::make_shared<T>(std::move(r)); }

using namespace cu;

struct BasicVertex { float3 pos, norm; float2 uv; };
struct NormalVertex { float3 pos, norm, tan, bitan; float2 uv; };
struct ColorVertex { float3 pos; ubyte4 col; };

#define FIELD(n) f(#n, o.n)
template<class F> void visit_fields(BasicVertex & o, F f) { FIELD(pos); FIELD(norm); FIELD(uv); }
template<class F> void visit_fields(ColorVertex & o, F f) { FIELD(pos); FIELD(col); }
template<class V, class I, class F> void visit_fields(TriMesh<V,I> & o, F f) { FIELD(verts); FIELD(tris); }
#undef FIELD

TriMesh<BasicVertex,uint8_t> basicBox(const float3 & dims) { return boxMesh(dims, &BasicVertex::pos, &BasicVertex::norm, &BasicVertex::uv); }

TriMesh<NormalVertex, uint8_t> normalBox(const float3 & dims)
{ 
    auto mesh = boxMesh(dims, &NormalVertex::pos, &NormalVertex::norm, &NormalVertex::uv);
    computeTangents(mesh.verts, mesh.tris, &NormalVertex::tan, &NormalVertex::bitan, &NormalVertex::pos, &NormalVertex::uv);
    return mesh;
}

TriMesh<ColorVertex,uint8_t> colorBox(const float3 & dims, const ubyte4 & col)
{
    auto mesh = boxMesh(dims, &ColorVertex::pos);
    setField(mesh.verts, &ColorVertex::col, col);
    return mesh;
}

template<class I> GlMesh glMesh(const TriMesh<BasicVertex,I> & mesh) { return GlMesh(mesh.verts, mesh.tris, &BasicVertex::pos, &BasicVertex::norm, nullptr, nullptr, &BasicVertex::uv); }
template<class I> GlMesh glMesh(const TriMesh<NormalVertex, I> & mesh) { return GlMesh(mesh.verts, mesh.tris, &NormalVertex::pos, &NormalVertex::norm, &NormalVertex::tan, &NormalVertex::bitan, &NormalVertex::uv); }
template<class I> GlMesh glMesh(const TriMesh<ColorVertex,I> & mesh) { return GlMesh(mesh.verts, mesh.tris, &ColorVertex::pos, &ColorVertex::col); }

int main(int argc, char * argv[])
{
    try
    {
        Window window("Graphics App", {640,480});
        window.WriteGlVersion(std::cout);

        auto unlitProg = shared(GlProgram(
            {GL_VERTEX_SHADER, {g_vertShaderPreamble, R"(
                layout(location = 0) in vec3 v_position;
                layout(location = 1) in vec4 v_color;
                out vec4 color;
                void main()
                {
                    setWorldPosition(transformCoord(pose,v_position));
                    color = v_color;
                }
            )"}},
            {GL_FRAGMENT_SHADER, {g_fragShaderPreamble, R"(
                in vec4 color;
                layout(location = 0) out vec4 f_color;
                void main()
                {
                    f_color = color;
                }
            )"}}
        ));

        // Program which renders only geometry for static meshes, useful for shadow mapping
        auto geoOnlyProg = shared(GlProgram(
            { GL_VERTEX_SHADER, {g_vertShaderPreamble, "layout(location = 0) in vec3 v_position;\nvoid main() { setWorldPosition(transformCoord(pose,v_position)); }"}},
            { GL_FRAGMENT_SHADER, {g_fragShaderPreamble, "void main() {}"}}));

        auto litProg = shared(GlProgram(
            {GL_VERTEX_SHADER, {g_vertShaderPreamble, R"(
                layout(location = 0) in vec3 v_position;
                layout(location = 1) in vec3 v_normal;
                layout(location = 2) in vec3 v_tangent;
                layout(location = 3) in vec3 v_bitangent;
                layout(location = 4) in vec2 v_texCoord;
                out vec3 position;
                out vec3 normal;
                out vec3 tangent;
                out vec3 bitangent;
                out vec2 texCoord;
                void main()
                {
                    position    = transformCoord(pose, v_position);
                    normal      = transformVector(pose, v_normal);
                    tangent     = transformVector(pose, v_tangent);
                    bitangent   = transformVector(pose, v_bitangent);
                    texCoord    = v_texCoord;
                    setWorldPosition(position);
                }
            )"}},
            {GL_FRAGMENT_SHADER, {g_fragShaderPreamble, R"(
                layout(binding = 0) uniform sampler2D u_texAlbedo;
                layout(binding = 1) uniform sampler2D u_texNormal;
                in vec3 position;
                in vec3 normal;
                in vec3 tangent;
                in vec3 bitangent;
                in vec2 texCoord;
                layout(location = 0) out vec4 f_color;   
                void main()
                {
                    vec3 tsNormal = texture(u_texNormal, texCoord).xyz*2 - 1;
                    vec3 vsNormal = normalize(normalize(tangent) * tsNormal.x + normalize(bitangent) * tsNormal.y + normalize(normal) * tsNormal.z);
                    vec4 albedo = texture(u_texAlbedo, texCoord);
                    f_color = vec4(albedo.rgb * computeLighting(position, vsNormal), albedo.a);
                }
            )"}}
        ));

        auto sampNearest = shared(GlSampler(GL_NEAREST, GL_NEAREST, GL_REPEAT, false));
        auto sampLinear = shared(GlSampler(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, false));

        const ubyte4 checkerboardPixels[] = { { 32, 32, 32, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 32, 32, 32, 255 } }, flatNormal = { 127, 127, 255, 255 };
        Material matCheckerboard = { litProg, geoOnlyProg, shared(GlTexture(GL_RGBA, { 2, 2 }, 1, checkerboardPixels)), shared(GlTexture(GL_RGBA, { 1, 1 }, 1, &flatNormal)), sampNearest };
        Material matGreenwall = { litProg, geoOnlyProg, shared(loadTextureFromDdsFile("../../assets/greenwall_albedo.dds")), shared(loadTextureFromDdsFile("../../assets/greenwall_normal.dds")), sampLinear };
        Material matLight = {unlitProg, geoOnlyProg, 0, 0};

        auto brickBox = shared(glMesh(normalBox({ 1.2f, 1.0f, 0.8f })));
        auto groundBox = shared(glMesh(normalBox(float3(8, 1, 8))));

        auto lightBox0 = shared(glMesh(colorBox({ 0.2f, 0.2f, 0.2f }, { 255, 255, 192, 255 })));
        auto lightBox1 = shared(glMesh(colorBox({ 0.2f, 0.2f, 0.2f }, { 255,  64,  64, 255 })));

        // Define scene
        std::vector<Object> objs = {
            { matCheckerboard, groundBox, float3(0, -2, -4) },
            { matLight, lightBox0, Pose(float3(0, 4, -10), qmul(qrotation(float3(1, 0, 0), 0.8f), qrotation(float3(0, 1, 0), 6.28f / 2))) },
            { matLight, lightBox1, Pose(float3(0, 4, 2), qrotation(float3(1, 0, 0), -0.8f)) },
            { matGreenwall, brickBox, float3(-2, 0, -2) },
            { matGreenwall, brickBox, float3(-2, 0, -6) },
            { matGreenwall, brickBox, float3(+2, 0, -2) },
            { matGreenwall, brickBox, float3(+2, 0, -6) }
        };

        auto fbScreen = GlFramebuffer(uint2(640,480));

        float t=0;
        View view = { 1.0f, 0.1f, 16.0f, Pose() };

        std::vector<Light> lights = {
            { {1,1.00f,0.75f}, {1.0f, 0.2f, 16.0f, objs[1].pose} },
            { {1,0.25f,0.25f}, {1.0f, 0.2f, 16.0f, objs[2].pose} }
        };

        Renderer renderer;

        bool quit = false, run = true;
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
                    if (e.key.keysym.sym == SDLK_ESCAPE) quit = true;
                    if (e.key.keysym.sym == SDLK_SPACE) run = !run;
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

            float3 move;
            if(w) move.z -= 1;
            if(a) move.x -= 1;
            if(s) move.z += 1;
            if(d) move.x += 1;
            view.pose.orientation = qmul(qrotation(float3(0,1,0), yaw), qrotation(float3(1,0,0), pitch));
            view.pose.position = view.pose * (safenorm(move)*(timestep * 10));
            
            if (run)
            {
                t += timestep * 2;
                objs[3].pose.orientation = qrotation(float3(1, 0, 0), t);
                objs[4].pose.orientation = qrotation(float3(0, 1, 0), t);
                objs[5].pose.orientation = qrotation(float3(0, 0, 1), t);
            }

            renderer.render(fbScreen, view, objs, lights);

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