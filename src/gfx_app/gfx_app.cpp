#include <cu/geom.h>
#include <cu/mesh.h>
#include <cu/plat.h>
#include <cu/refl.h>
#include <cu/load.h>

#include <vector>
#include <iostream>
#include <memory>

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

struct Material
{
    std::shared_ptr<GlProgram> prog, shadowProg;
    std::shared_ptr<const GlTexture> texAlbedo, texNormal;
    std::shared_ptr<const GlSampler> samp;
};

struct Object
{
    Material mat;
    std::shared_ptr<const GlMesh> mesh;
    Pose pose;
};

void renderScene(const std::vector<Object> & objs, const Pose & camPose, float aspect, const GlTexture * depthTex, const GlSampler & sampShadow, bool renderDepth, const UniformBlockDesc & block, GlUniformBuffer & ubo, 
    const UniformBlockDesc & tblock, GlUniformBuffer & tubo);

int main(int argc, char * argv[])
{
    try
    {
        Window window("Graphics App", {640,480});
        window.WriteGlVersion(std::cout);

        auto unlitProg = shared(GlProgram(
            {GL_VERTEX_SHADER, R"(
                #version 330
                layout(shared, binding = 0) uniform Transform
                {
                    mat4 matClipFromModel;
                    mat4 matViewFromModel;
                };
                layout(location = 0) in vec3 v_position;
                layout(location = 1) in vec4 v_color;
                out vec4 color;
                void main()
                {
                    gl_Position = matClipFromModel * vec4(v_position,1);
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
        ));

        // Program which renders only geometry for static meshes, useful for shadow mapping
        auto geoOnlyProg = shared(GlProgram(
            { GL_VERTEX_SHADER,   R"(
                #version 330
                layout(shared, binding = 0) uniform Transform
                {
                    mat4 matClipFromModel;
                    mat4 matViewFromModel;
                };
                layout(location = 0) in vec3 v_position;
                void main() { gl_Position = matClipFromModel * vec4(v_position,1); }
            )"},
            { GL_FRAGMENT_SHADER, "#version 330\n void main() {}" }));

        auto litProg = shared(GlProgram(
            {GL_VERTEX_SHADER, R"(
                #version 330
                layout(shared, binding = 0) uniform Transform
                {
                    mat4 matClipFromModel;
                    mat4 matViewFromModel;
                };
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
                    gl_Position = matClipFromModel * vec4(v_position,1);
                    position    = (matViewFromModel * vec4(v_position,1)).xyz; // Assume transform outputs w=1
                    normal      = (matViewFromModel * vec4(v_normal,0)).xyz;
                    tangent     = (matViewFromModel * vec4(v_tangent,0)).xyz;
                    bitangent   = (matViewFromModel * vec4(v_bitangent,0)).xyz;
                    texCoord    = v_texCoord;
                }
            )"},
            {GL_FRAGMENT_SHADER, R"(
                #version 330
                layout(binding = 0) uniform sampler2D u_texAlbedo;
                layout(binding = 1) uniform sampler2D u_texNormal;
                layout(binding = 8) uniform sampler2DShadow	u_texShadow;
                layout(shared, binding = 4) uniform Lighting
                {
                    vec3 ambient;
                    vec3 lightPos;
                    mat4 lightMatrix;
                };
                in vec3 position;
                in vec3 normal;
                in vec3 tangent;
                in vec3 bitangent;
                in vec2 texCoord;
                layout(location = 0) out vec4 f_color;
                void main()
                {
                    vec4 lsPos = mul(lightMatrix,vec4(position,1));
		            vec3 lightColor = vec3(1,1,1) * shadow2DProj(u_texShadow, lsPos).r;

                    vec3 tsNormal = texture(u_texNormal, texCoord).xyz*2 - 1;
                    vec3 vsNormal = normalize(normalize(tangent) * tsNormal.x + normalize(bitangent) * tsNormal.y + normalize(normal) * tsNormal.z);

                    vec3 lightDir = normalize(lightPos - position);
                    vec3 eyeDir   = normalize(-position);
                    vec3 halfDir  = normalize(lightDir + eyeDir);

                    vec3 light = ambient; //vec3(0.2,0.2,0.2);
                    light += lightColor * 0.8 * max(dot(lightDir, vsNormal), 0);
                    light += lightColor * pow(max(dot(halfDir, vsNormal), 0), 256);

                    vec4 albedo = texture(u_texAlbedo, texCoord);
                    f_color = vec4(albedo.rgb * light, albedo.a);
                }
            )"}
        ));

        auto sampNearest = shared(GlSampler(GL_NEAREST, GL_NEAREST, GL_REPEAT, false));
        auto sampLinear = shared(GlSampler(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, false));
        auto sampShadow = shared(GlSampler(GL_LINEAR, GL_LINEAR, GL_CLAMP, true));

        const ubyte4 checkerboardPixels[] = { { 32, 32, 32, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 32, 32, 32, 255 } }, flatNormal = { 127, 127, 255, 255 };
        Material matCheckerboard = { litProg, geoOnlyProg, shared(GlTexture(GL_RGBA, { 2, 2 }, 1, checkerboardPixels)), shared(GlTexture(GL_RGBA, { 1, 1 }, 1, &flatNormal)), sampNearest };
        Material matGreenwall = { litProg, geoOnlyProg, shared(loadTextureFromDdsFile("../../assets/greenwall_albedo.dds")), shared(loadTextureFromDdsFile("../../assets/greenwall_normal.dds")), sampLinear };
        Material matLight = {unlitProg, geoOnlyProg, 0, 0};

        auto brickBox = shared(glMesh(normalBox({ 1.2f, 1.0f, 0.8f })));
        auto groundBox = shared(glMesh(normalBox(float3(8, 1, 8))));
        auto lightBox = shared(glMesh(colorBox({ 0.2f, 0.2f, 0.2f }, { 255, 255, 127, 255 })));

        // Define scene
        std::vector<Object> objs = {
            { matCheckerboard, groundBox, float3(0, -2, -4) },
            { matLight, lightBox, Pose(float3(0, 4, -10), qmul(qrotation(float3(1,0,0), 0.8f), qrotation(float3(0,1,0), 6.28f/2))) },
            { matGreenwall, brickBox, float3(-2, 0, -2) },
            { matGreenwall, brickBox, float3(-2, 0, -6) },
            { matGreenwall, brickBox, float3(+2, 0, -2) },
            { matGreenwall, brickBox, float3(+2, 0, -6) }
        };

        auto fbScreen = GlFramebuffer(uint2(640,480));
        auto fbShadow = GlFramebuffer::shadowBuffer(uint2(256,256));

        // Report on uniforms
        for (auto & samp : litProg->description().samplers)
        {
            std::cout << "sampler " << samp.name << " {binding=" << samp.binding << "}" << std::endl;
        }

        auto & block = litProg->description().blocks.front();
        std::cout << "(0) " << block.name << " : byte[" << block.pack.size << "] {binding=" << block.binding << "}" << std::endl;
        for (auto & un : block.pack.fields)
        {
            const char * types[] = { "float", "double", "int", "uint", "bool" };
            std::cout << "(0:" << un.offset << ") " << un.name << " : " << types[un.baseType];
            if (un.dimensions.x > 1) std::cout << un.dimensions.x;
            if (un.dimensions.y > 1) std::cout << 'x' << un.dimensions.y;
            if (un.dimensions.z > 1) std::cout << '[' << un.dimensions.z << ']';
            std::cout << " {stride=" << toJson(un.stride) << '}' << std::endl;
        }

        auto transBlock = litProg->block("Transform");
        auto lightBlock = litProg->block("Lighting");
        GlUniformBuffer ubo,ubo2;

        float t=0;
        Pose camPose;

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
            camPose.orientation = qmul(qrotation(float3(0,1,0), yaw), qrotation(float3(1,0,0), pitch));
            camPose.position = camPose * (safenorm(move)*(timestep*10));
            
            if (run)
            {
                t += timestep * 2;
                objs[2].pose.orientation = qrotation(float3(1, 0, 0), t);
                objs[3].pose.orientation = qrotation(float3(0, 1, 0), t);
                objs[4].pose.orientation = qrotation(float3(0, 0, 1), t);
            }

            fbShadow.bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene(objs, objs[1].pose, 1.0f, 0, *sampShadow, true, *lightBlock, ubo, *transBlock, ubo2);

            fbScreen.bind();
            glClearColor(0.2f, 0.6f, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene(objs, camPose, 1.333f, &fbShadow.texture(0), *sampShadow, false, *lightBlock, ubo, *transBlock, ubo2);

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

void renderScene(const std::vector<Object> & objs, const Pose & camPose, float aspect, const GlTexture * depthTex, const GlSampler & sampShadow, bool renderDepth, const UniformBlockDesc & block, GlUniformBuffer & ubo,
    const UniformBlockDesc & tblock, GlUniformBuffer & tubo)
{
    auto lightPose = objs[1].pose;

    auto clipFromView = perspectiveMatrixRhGl(1.0f, aspect, 0.1f, 16.0f);
    auto viewFromWorld = camPose.inverse().matrix();

    auto shadowClipFromView = perspectiveMatrixRhGl(1.0f, 1.0f, 0.1f, 16.0f);
    auto shadowViewFromWorld = lightPose.inverse().matrix();

    // Compute matrix that goes from view space to biased shadow clip space
    auto shadowTexFromClip = float4x4{{0.5f, 0, 0, 0}, {0, 0.5f, 0, 0}, {0, 0, 0.5f, 0}, {0.5f, 0.5f, 0.5f, 1}};
    auto worldFromView = camPose.matrix();
    auto shadowTexFromView = mul(shadowTexFromClip, shadowClipFromView, shadowViewFromWorld, worldFromView);

    // Set up lighting UBO
    std::vector<uint8_t> buffer(block.pack.size);
    block.set(buffer.data(), "ambient", float3(0.2, 0.2, 0.2));
    block.set(buffer.data(), "lightPos", transformCoord(viewFromWorld, lightPose.position));
    block.set(buffer.data(), "lightMatrix", shadowTexFromView);
    ubo.setData(buffer.data(), buffer.size(), GL_STREAM_DRAW);
    ubo.bind(block.binding);

    std::vector<uint8_t> tbuffer(tblock.pack.size);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    if(!renderDepth) glCullFace(GL_BACK);
    else glCullFace(GL_FRONT);

    for (auto & obj : objs)
    {
        auto viewFromModel = mul(viewFromWorld, obj.pose.matrix());

        const auto & prog = depthTex ? *obj.mat.prog : *obj.mat.shadowProg;

        tblock.set(tbuffer.data(), "matClipFromModel", mul(clipFromView, viewFromModel));
        tblock.set(tbuffer.data(), "matViewFromModel", viewFromModel);
        tubo.setData(tbuffer.data(), tbuffer.size(), GL_DYNAMIC_DRAW);
        tubo.bind(tblock.binding);

        prog.use();
        if (depthTex)
        {
            if (obj.mat.texAlbedo) obj.mat.texAlbedo->bind(0, *obj.mat.samp);
            if (obj.mat.texNormal) obj.mat.texNormal->bind(1, *obj.mat.samp);
            depthTex->bind(8, sampShadow);
        }

        obj.mesh->draw();
    }
}