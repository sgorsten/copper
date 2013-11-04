#ifndef RENDER_H
#define RENDER_H

#include "cu/draw.h"
#include "cu/geom.h"
#include <memory>

using namespace cu;

struct Material
{
    std::shared_ptr<const GlProgram> prog, shadowProg;
    std::shared_ptr<const GlTexture> texAlbedo, texNormal;
    std::shared_ptr<const GlSampler> samp;
};

struct Object
{
    Material mat;
    std::shared_ptr<const GlMesh> mesh;
    Pose pose;
};

extern const char * g_vertShaderPreamble;
extern const char * g_fragShaderPreamble;

class Renderer
{
    GlProgram blockReference;
    const UniformBlockDesc * transformBlock, * lightingBlock;
    GlUniformBuffer transformUbo, lightingUbo;
    GlFramebuffer shadowBuffer;
    GlSampler shadowSampler;
public:
    Renderer();

    void renderScene(const std::vector<Object> & objs, const Pose & camPose, float aspect, bool renderDepth);

    void render(GlFramebuffer & screen, const std::vector<Object> & objs, const Pose & camPose);
};

#endif