#include "render.h"

const char * g_vertShaderPreamble = R"(
    #version 420
    layout(shared, binding = 0) uniform Transform
    {
        mat4 matClipFromModel;
        mat4 matViewFromModel;
    };
)";

const char * g_fragShaderPreamble = R"(
    #version 420
    layout(shared, binding = 4) uniform Lighting
    {
        vec3 ambient;
        vec3 lightColor;
        vec3 lightPos;
        mat4 lightMatrix;
    };
    layout(binding = 8) uniform sampler2DShadow	u_texShadow;

    vec3 computeLighting(vec3 vsPosition, vec3 vsNormal)
    {
        vec3 eyeDir     = normalize(-vsPosition);
        vec3 light      = ambient;

        vec4 lsPos      = lightMatrix * vec4(vsPosition,1);
        vec3 lightAmt   = lightColor * textureProj(u_texShadow, lsPos).r;

        vec3 lightDir   = normalize(lightPos - vsPosition);
        vec3 halfDir    = normalize(lightDir + eyeDir);

        light += lightAmt * 0.8 * max(dot(lightDir, vsNormal), 0);
        light += lightAmt * pow(max(dot(halfDir, vsNormal), 0), 256);

        return light;
    }
)";

Renderer::Renderer()
{
    blockReference = GlProgram(
        { GL_VERTEX_SHADER, { g_vertShaderPreamble, "void main() { gl_Position = matClipFromModel * vec4(0,0,0,1); }" } },
        { GL_FRAGMENT_SHADER, { g_fragShaderPreamble, "layout(location = 0) out vec4 f_color; void main() { f_color = vec4(ambient,1); }" } }
    );

    transformBlock = blockReference.block("Transform");
    transformUbo = GlUniformBuffer(transformBlock->pack.size, GL_DYNAMIC_DRAW);
    transformUbo.bind(transformBlock->binding);

    lightingBlock = blockReference.block("Lighting");   
    lightingUbo = GlUniformBuffer(lightingBlock->pack.size, GL_STREAM_DRAW);
    lightingUbo.bind(lightingBlock->binding);

    shadowBuffer = GlFramebuffer::shadowBuffer(uint2(512,512));
    shadowSampler = GlSampler(GL_LINEAR, GL_LINEAR, GL_CLAMP, true);
}    

void Renderer::render(GlFramebuffer & screen, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights)
{
    renderScene(shadowBuffer, lights[0].view, objs, lights, true);

    glClearColor(0.2f, 0.6f, 1, 1);
    renderScene(screen, view, objs, lights, false);
}

static float4x4 matClipFromView(const View & view, float aspect) { return perspectiveMatrixRhGl(view.vfov, aspect, view.nearClip, view.farClip); }
static float4x4 matViewFromWorld(const View & view) { return view.pose.inverse().matrix(); }
static float aspectFromFramebuffer(const GlFramebuffer & fb) { auto dims = fb.dimensions(); return (float)dims.x/dims.y; }

void Renderer::renderScene(GlFramebuffer & target, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth)
{
    auto clipFromView = matClipFromView(view, aspectFromFramebuffer(target));
    auto viewFromWorld = matViewFromWorld(view);

    auto shadowClipFromView = matClipFromView(lights[0].view, aspectFromFramebuffer(shadowBuffer));
    auto shadowViewFromWorld = matViewFromWorld(lights[0].view);

    // Compute matrix that goes from view space to biased shadow clip space
    auto shadowTexFromClip = float4x4{ { 0.5f, 0, 0, 0 }, { 0, 0.5f, 0, 0 }, { 0, 0, 0.5f, 0 }, { 0.5f, 0.5f, 0.5f, 1 } };
    auto worldFromView = view.pose.matrix();
    auto shadowTexFromView = mul(shadowTexFromClip, shadowClipFromView, shadowViewFromWorld, worldFromView);

    // Set up lighting UBO
    std::vector<uint8_t> buffer(lightingBlock->pack.size);
    lightingBlock->set(buffer, "ambient", float3(0.2, 0.2, 0.2));
    lightingBlock->set(buffer, "lightColor", lights[0].color);
    lightingBlock->set(buffer, "lightPos", transformCoord(viewFromWorld, lights[0].view.pose.position));
    lightingBlock->set(buffer, "lightMatrix", shadowTexFromView);
    lightingUbo.setData(buffer, GL_STREAM_DRAW);

    std::vector<uint8_t> tbuffer(transformBlock->pack.size);

    target.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(renderDepth ? GL_FRONT : GL_BACK);
    for (auto & obj : objs)
    {
        // Set up transform
        auto viewFromModel = mul(viewFromWorld, obj.pose.matrix());
        transformBlock->set(tbuffer, "matClipFromModel", mul(clipFromView, viewFromModel));
        transformBlock->set(tbuffer, "matViewFromModel", viewFromModel);
        transformUbo.setData(tbuffer, GL_DYNAMIC_DRAW);

        // Bind program and render
        const auto & prog = renderDepth ? *obj.mat.shadowProg : *obj.mat.prog;
        prog.use();
        if (!renderDepth)
        {
            if (obj.mat.texAlbedo) obj.mat.texAlbedo->bind(0, *obj.mat.samp);
            if (obj.mat.texNormal) obj.mat.texNormal->bind(1, *obj.mat.samp);
            shadowBuffer.texture(0).bind(8, shadowSampler);
        }
        obj.mesh->draw();
    }
}