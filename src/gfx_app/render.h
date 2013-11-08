#ifndef RENDER_H
#define RENDER_H

#include "cu/draw.h"
#include "cu/geom.h"
#include <memory>

using namespace cu;

struct View
{
    float vfov, nearClip, farClip;
    Pose pose;
};

struct Light
{
    float3 color;
    View view;
};

struct Material
{
    struct SamplerBinding { GLuint unit; std::shared_ptr<const GlTexture> tex; std::shared_ptr<const GlSampler> samp; };

    std::shared_ptr<const GlProgram> prog, shadowProg;
    const UniformBlockDesc * perObjectBlock;
    std::vector<uint8_t> uniformBindings;
    std::vector<SamplerBinding> samplerBindings;

    Material(std::shared_ptr<const GlProgram> prog, std::shared_ptr<const GlProgram> shadowProg) 
        : prog(prog), shadowProg(shadowProg), perObjectBlock(prog->block("PerObject")), uniformBindings(perObjectBlock ? perObjectBlock->pack.size : 0) {}

    template<class T> void set(const std::string & uniformName, const T & value) { if(perObjectBlock) perObjectBlock->set(uniformBindings, uniformName, value); }
    void set(const std::string & samplerName, std::shared_ptr<const GlTexture> tex, std::shared_ptr<const GlSampler> samp)
    {
        if (auto s = prog->sampler(samplerName.c_str())) samplerBindings.push_back({ s->binding, move(tex), move(samp) }); 
    }
};

struct Object
{
    Material mat;
    std::shared_ptr<const GlMesh> mesh;
    Pose pose;
};

extern const char * g_shaderPreamble;
extern const char * g_vertShaderPreamble;
extern const char * g_fragShaderPreamble;

class Renderer
{
    GlProgram blockReference;
    const UniformBlockDesc * perSceneBlock, * perViewBlock;
    GlUniformBuffer perSceneUbo, perViewUbo, perObjectUbo;
    GlFramebuffer shadowBuffers[2];
    GlSampler shadowSampler;

    void renderScene(GlFramebuffer & target, const View & view, const std::vector<size_t> & perObjectOffsets, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth);
public:
    Renderer();

    void render(GlFramebuffer & screen, const View & view, const std::vector<Object> & objects, const std::vector<Light> & lights);
};

#endif