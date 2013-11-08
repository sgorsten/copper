#include "render.h"

const char * g_shaderPreamble = R"(
    #version 420

    // Quaternions
    vec4 qmul(vec4 a, vec4 b) { return vec4(a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y, a.y*b.w+a.w*b.y+a.z*b.x-a.x*b.z, a.z*b.w+a.w*b.z+a.x*b.y-a.y*b.x, a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z); }  
    vec3 qtransform(vec4 q, vec3 v) { 
        vec4 a = q*q, b = q*q.yzwx; vec2 c = q.xy*q.zw;
        return vec3(a.w+a.x-a.y-a.z, (b.x+b.z)*2, (c.x-c.y)*2)*v.x 
             + vec3((b.x-b.z)*2, a.w-a.x+a.y-a.z, (b.y+b.w)*2)*v.y 
             + vec3((c.x+c.y)*2, (b.y-b.w)*2, a.w-a.x-a.y+a.z)*v.z; 
    }

    // Poses
    struct Pose { vec3 position; vec4 orientation; };
    vec3 transformVector(Pose pose, vec3 vector) { return qtransform(pose.orientation, vector); }
    vec3 transformCoord(Pose pose, vec3 coord) { return pose.position + transformVector(pose, coord); }

    // Uniform block specified per view (cameras, shadow buffers, etc.)
    layout(shared, binding = 9) uniform PerView
    {
        mat4 matClipFromWorld;
        vec3 eyePosition;
    };
)";

const char * g_vertShaderPreamble = R"(
    void setWorldPosition(vec3 position) { gl_Position = matClipFromWorld * vec4(position,1); }
)";

const char * g_fragShaderPreamble = R"(
    struct ShadowLight
    {
        mat4 matrix;
        vec3 position;
        vec3 color;
    };
    layout(shared, binding = 8) uniform PerScene
    {
        ShadowLight shadowLights[2];
        vec3 ambientLight;
    };
    layout(binding = 8) uniform sampler2DShadow	u_texShadow[2];

    vec3 computeLighting(vec3 wsPosition, vec3 wsNormal)
    {
        vec3 eyeDir     = normalize(eyePosition - wsPosition);
        vec3 light      = ambientLight;

        for(int i=0; i<2; ++i)
        {
            vec4 lsPos      = shadowLights[i].matrix * vec4(wsPosition,1);
            vec3 lightAmt   = shadowLights[i].color * textureProj(u_texShadow[i], lsPos).r;

            vec3 lightDir   = normalize(shadowLights[i].position - wsPosition);
            vec3 halfDir    = normalize(lightDir + eyeDir);

            light += lightAmt * 0.8 * max(dot(lightDir, wsNormal), 0);
            light += lightAmt * pow(max(dot(halfDir, wsNormal), 0), 256);
        }

        return light;
    }
)";

Renderer::Renderer()
{
    blockReference = GlProgram(
        { GL_VERTEX_SHADER, { g_shaderPreamble, g_vertShaderPreamble, "void main() { setWorldPosition(vec3(0,0,0)); }" } },
        { GL_FRAGMENT_SHADER, { g_shaderPreamble, g_fragShaderPreamble, "layout(location = 0) out vec4 f_color; void main() { f_color = vec4(ambientLight,1); }" } }
    );

    perSceneBlock = blockReference.block("PerScene");
    perSceneUbo = GlUniformBuffer(perSceneBlock->pack.size, GL_STREAM_DRAW);

    perViewBlock = blockReference.block("PerView");
    perViewUbo = GlUniformBuffer(perViewBlock->pack.size, GL_STREAM_DRAW);

    shadowBuffers[0] = GlFramebuffer::shadowBuffer(uint2(512, 512));
    shadowBuffers[1] = GlFramebuffer::shadowBuffer(uint2(512, 512));
    shadowSampler = GlSampler(GL_LINEAR, GL_LINEAR, GL_CLAMP, true);
}    

static float4x4 matClipFromWorld(const GlFramebuffer & fb, const View & view) { auto dims = fb.dimensions(); return mul(perspectiveMatrixRhGl(view.vfov, (float)dims.x/dims.y, view.nearClip, view.farClip), view.pose.inverse().matrix()); }
void Renderer::render(GlFramebuffer & screen, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights)
{
    // Set up PerObject uniform block
    GLint uboAlignment; glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignment);
    std::vector<size_t> perObjectOffsets = { 0 };
    for (auto & obj : objs) perObjectOffsets.push_back(((perObjectOffsets.back() + (obj.mat.perObjectBlock ? obj.mat.perObjectBlock->pack.size : 0) + uboAlignment-1)/uboAlignment)*uboAlignment);
    std::vector<uint8_t> pobuffer(perObjectOffsets.back());
    for (size_t i = 0; i < objs.size(); ++i)
    {
        std::copy(begin(objs[i].mat.uniformBindings), end(objs[i].mat.uniformBindings), begin(pobuffer) + perObjectOffsets[i]);
        objs[i].mat.perObjectBlock->set(pobuffer.data() + perObjectOffsets[i], "pose.position", objs[i].pose.position);
        objs[i].mat.perObjectBlock->set(pobuffer.data() + perObjectOffsets[i], "pose.orientation", objs[i].pose.orientation);
    }
    perObjectUbo.setData(pobuffer, GL_STREAM_DRAW);

    // Render shadow buffers
    renderScene(shadowBuffers[0], lights[0].view, perObjectOffsets, objs, lights, true);
    renderScene(shadowBuffers[1], lights[1].view, perObjectOffsets, objs, lights, true);

    // Set up PerScene uniform block
    std::vector<uint8_t> psbuffer(perSceneBlock->pack.size);
    perSceneBlock->set(psbuffer, "ambientLight", float3(0.05f, 0.05f, 0.05f));
    const auto shadowTexFromClip = float4x4{ { 0.5f, 0, 0, 0 }, { 0, 0.5f, 0, 0 }, { 0, 0, 0.5f, 0 }, { 0.5f, 0.5f, 0.5f, 1 } };
    for (int i = 0; i<2; ++i)
    {
        std::ostringstream ss; ss << "shadowLights[" << i << "]."; auto prefix = ss.str();
        perSceneBlock->set(psbuffer, prefix + "color", lights[i].color);
        perSceneBlock->set(psbuffer, prefix + "position", lights[i].view.pose.position);
        perSceneBlock->set(psbuffer, prefix + "matrix", mul(shadowTexFromClip, matClipFromWorld(shadowBuffers[i], lights[i].view)));
        shadowBuffers[i].texture(0).bind(8 + i, shadowSampler);
    }
    perSceneUbo.setData(psbuffer, GL_STREAM_DRAW);
    perSceneUbo.bind(perSceneBlock->binding);

    // Render final scene
    glClearColor(0.2f, 0.6f, 1, 1);
    renderScene(screen, view, perObjectOffsets, objs, lights, false);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void Renderer::renderScene(GlFramebuffer & target, const View & view, const std::vector<size_t> & perObjectOffsets, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth)
{
    // Set up PerView uniform block
    std::vector<uint8_t> pvbuffer(perViewBlock->pack.size);
    perViewBlock->set(pvbuffer, "matClipFromWorld", matClipFromWorld(target, view));
    perViewBlock->set(pvbuffer, "eyePosition", view.pose.position);
    perViewUbo.setData(pvbuffer, GL_STREAM_DRAW);
    perViewUbo.bind(perViewBlock->binding);

    // Set up target and common GL state
    target.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(renderDepth ? GL_FRONT : GL_BACK);

    // Render objects
    for (size_t i=0; i<objs.size(); ++i)
    {
        if (auto prog = renderDepth ? objs[i].mat.shadowProg.get() : objs[i].mat.prog.get())
        {
            if (objs[i].mat.perObjectBlock) perObjectUbo.bind(objs[i].mat.perObjectBlock->binding, perObjectOffsets[i], perObjectOffsets[i + 1] - perObjectOffsets[i]);
            if (!renderDepth) for (auto & binding : objs[i].mat.samplerBindings) binding.tex->bind(binding.unit, *binding.samp);
            prog->use();
            objs[i].mesh->draw();
        }
    }
}