#include "render.h"

const char * g_shaderPreamble = R"(
    #version 420

    // Quaternions
    vec4 qmul(vec4 a, vec4 b) { return vec4(a.x*b.w+a.w*b.x+a.y*b.z-a.z*b.y, a.y*b.w+a.w*b.y+a.z*b.x-a.x*b.z, a.z*b.w+a.w*b.z+a.x*b.y-a.y*b.x, a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z); }  
    vec3 qxdir(vec4 q) { return vec3(q.w*q.w+q.x*q.x-q.y*q.y-q.z*q.z, (q.x*q.y+q.z*q.w)*2, (q.z*q.x-q.y*q.w)*2); } 
    vec3 qydir(vec4 q) { return vec3((q.x*q.y-q.z*q.w)*2, q.w*q.w-q.x*q.x+q.y*q.y-q.z*q.z, (q.y*q.z+q.x*q.w)*2); } 
    vec3 qzdir(vec4 q) { return vec3((q.z*q.x+q.y*q.w)*2, (q.y*q.z-q.x*q.w)*2, q.w*q.w-q.x*q.x-q.y*q.y+q.z*q.z); } 
    vec3 qtransform(vec4 q, vec3 v) { return qxdir(q)*v.x + qydir(q)*v.y + qzdir(q)*v.z; }

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
    layout(shared, binding = 0) uniform Transform
    {
        Pose pose;
    };
    void setWorldPosition(vec3 position) { gl_Position = matClipFromWorld * vec4(position,1); }
)";

const char * g_fragShaderPreamble = R"(
    struct ShadowLight
    {
        mat4 matrix;
        vec3 position;
        vec3 color;
    };
    layout(shared, binding = 4) uniform Lighting
    {
        ShadowLight shadowLights[2];
        vec3 ambient;
    };
    layout(binding = 8) uniform sampler2DShadow	u_texShadow[2];

    vec3 computeLighting(vec3 wsPosition, vec3 wsNormal)
    {
        vec3 eyeDir     = normalize(eyePosition - wsPosition);
        vec3 light      = ambient;

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
        { GL_VERTEX_SHADER, { g_shaderPreamble, g_vertShaderPreamble, "void main() { setWorldPosition(transformCoord(pose,vec3(0,0,0))); }" } },
        { GL_FRAGMENT_SHADER, { g_shaderPreamble, g_fragShaderPreamble, "layout(location = 0) out vec4 f_color; void main() { f_color = vec4(ambient,1); }" } }
    );

    perViewBlock = blockReference.block("PerView");
    perViewUbo = GlUniformBuffer(perViewBlock->pack.size, GL_STREAM_DRAW);
    perViewUbo.bind(perViewBlock->binding);

    transformBlock = blockReference.block("Transform");
    transformUbo = GlUniformBuffer(transformBlock->pack.size, GL_DYNAMIC_DRAW);
    transformUbo.bind(transformBlock->binding);

    lightingBlock = blockReference.block("Lighting");   
    lightingUbo = GlUniformBuffer(lightingBlock->pack.size, GL_STREAM_DRAW);
    lightingUbo.bind(lightingBlock->binding);

    shadowBuffer[0] = GlFramebuffer::shadowBuffer(uint2(512,512));
    shadowBuffer[1] = GlFramebuffer::shadowBuffer(uint2(512, 512));
    shadowSampler = GlSampler(GL_LINEAR, GL_LINEAR, GL_CLAMP, true);
}    

void Renderer::render(GlFramebuffer & screen, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights)
{
    renderScene(shadowBuffer[0], lights[0].view, objs, lights, true);
    renderScene(shadowBuffer[1], lights[1].view, objs, lights, true);

    glClearColor(0.2f, 0.6f, 1, 1);
    renderScene(screen, view, objs, lights, false);
}

static float4x4 matClipFromWorld(const View & view, float aspect) { return mul(perspectiveMatrixRhGl(view.vfov, aspect, view.nearClip, view.farClip), view.pose.inverse().matrix()); }
static float aspectFromFramebuffer(const GlFramebuffer & fb) { auto dims = fb.dimensions(); return (float)dims.x/dims.y; }

void Renderer::renderScene(GlFramebuffer & target, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth)
{
    // Set up PerView uniform block
    std::vector<uint8_t> pvbuffer(perViewBlock->pack.size);
    perViewBlock->set(pvbuffer, "matClipFromWorld", matClipFromWorld(view, aspectFromFramebuffer(target)));
    perViewBlock->set(pvbuffer, "eyePosition", view.pose.position);
    perViewUbo.setData(pvbuffer, GL_STREAM_DRAW);

    // Set up lighting UBO
    if (!renderDepth)
    {
        std::vector<uint8_t> buffer(lightingBlock->pack.size);
        lightingBlock->set(buffer, "ambient", float3(0.05f, 0.05f, 0.05f));
        lightingBlock->set(buffer, "eyePos", view.pose.position);

        const auto worldFromView = view.pose.matrix();
        const auto shadowTexFromClip = float4x4{ { 0.5f, 0, 0, 0 }, { 0, 0.5f, 0, 0 }, { 0, 0, 0.5f, 0 }, { 0.5f, 0.5f, 0.5f, 1 } };
        for(int i=0; i<2; ++i)
        {
            std::ostringstream ss; ss << "shadowLights[" << i << "]."; auto prefix = ss.str();
            lightingBlock->set(buffer, prefix + "color", lights[i].color);
            lightingBlock->set(buffer, prefix + "position", lights[i].view.pose.position);
            lightingBlock->set(buffer, prefix + "matrix", mul(shadowTexFromClip, matClipFromWorld(lights[i].view, aspectFromFramebuffer(shadowBuffer[i]))));
            shadowBuffer[i].texture(0).bind(8+i, shadowSampler);
        }

        lightingUbo.setData(buffer, GL_STREAM_DRAW);
    }

    std::vector<uint8_t> tbuffer(transformBlock->pack.size);

    target.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(renderDepth ? GL_FRONT : GL_BACK);
    for (auto & obj : objs)
    {
        // Set up transform
        transformBlock->set(tbuffer, "pose.position",    obj.pose.position);
        transformBlock->set(tbuffer, "pose.orientation", obj.pose.orientation);
        transformUbo.setData(tbuffer, GL_DYNAMIC_DRAW);

        // Bind program and render
        const auto & prog = renderDepth ? *obj.mat.shadowProg : *obj.mat.prog;
        prog.use();
        if (!renderDepth)
        {
            if (obj.mat.texAlbedo) obj.mat.texAlbedo->bind(0, *obj.mat.samp);
            if (obj.mat.texNormal) obj.mat.texNormal->bind(1, *obj.mat.samp);
        }
        obj.mesh->draw();
    }
}