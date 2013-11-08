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
    std::shared_ptr<const GlProgram> prog, shadowProg;
    std::shared_ptr<const GlTexture> texAlbedo, texNormal;
    std::shared_ptr<const GlSampler> samp;
    const UniformBlockDesc * perObjectBlock;

    Material(std::shared_ptr<const GlProgram> prog, std::shared_ptr<const GlProgram> shadowProg, std::shared_ptr<const GlTexture> texAlbedo, std::shared_ptr<const GlTexture> texNormal, std::shared_ptr<const GlSampler> samp) 
        : prog(prog), shadowProg(shadowProg), texAlbedo(texAlbedo), texNormal(texNormal), samp(samp), perObjectBlock(prog->block("PerObject")) {}
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
    GlFramebuffer shadowBuffer[2];
    GlSampler shadowSampler;

    void renderScene(GlFramebuffer & target, const View & view, const std::vector<size_t> & perObjectOffsets, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth);
public:
    Renderer();

    void render(GlFramebuffer & screen, const View & view, const std::vector<Object> & objects, const std::vector<Light> & lights);
};

#endif