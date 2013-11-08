#include "render.h"

const char * g_vertShaderPreamble = R"(
    #version 420
    layout(shared, binding = 0) uniform Transform
    {
        mat4 matClipFromModel;
        mat4 matWorldFromModel;
    };
)";

const char * g_fragShaderPreamble = R"(
    #version 420
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
        vec3 eyePos;
    };
    layout(binding = 8) uniform sampler2DShadow	u_texShadow[2];

    vec3 computeLighting(vec3 wsPosition, vec3 wsNormal)
    {
        vec3 eyeDir     = normalize(eyePos - wsPosition);
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
        { GL_VERTEX_SHADER, { g_vertShaderPreamble, "void main() { gl_Position = matClipFromModel * vec4(0,0,0,1); }" } },
        { GL_FRAGMENT_SHADER, { g_fragShaderPreamble, "layout(location = 0) out vec4 f_color; void main() { f_color = vec4(ambient,1); }" } }
    );

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

static float4x4 matClipFromView(const View & view, float aspect) { return perspectiveMatrixRhGl(view.vfov, aspect, view.nearClip, view.farClip); }
static float4x4 matViewFromWorld(const View & view) { return view.pose.inverse().matrix(); }
static float aspectFromFramebuffer(const GlFramebuffer & fb) { auto dims = fb.dimensions(); return (float)dims.x/dims.y; }

void Renderer::renderScene(GlFramebuffer & target, const View & view, const std::vector<Object> & objs, const std::vector<Light> & lights, bool renderDepth)
{
    auto clipFromView = matClipFromView(view, aspectFromFramebuffer(target));
    auto viewFromWorld = matViewFromWorld(view);

    // Set up lighting UBO
    if (!renderDepth)
    {
        std::vector<uint8_t> buffer(lightingBlock->pack.size);
        lightingBlock->set(buffer, "ambient", float3(0.1, 0.1, 0.1));
        lightingBlock->set(buffer, "eyePos", view.pose.position);

        const auto worldFromView = view.pose.matrix();
        const auto shadowTexFromClip = float4x4{ { 0.5f, 0, 0, 0 }, { 0, 0.5f, 0, 0 }, { 0, 0, 0.5f, 0 }, { 0.5f, 0.5f, 0.5f, 1 } };
        for(int i=0; i<2; ++i)
        {
            // Compute prefix for light attributes
            std::ostringstream ss; ss << "shadowLights[" << i << "]."; auto prefix = ss.str();

            // Compute matrix that goes from view space to biased shadow clip space
            auto shadowClipFromView = matClipFromView(lights[i].view, aspectFromFramebuffer(shadowBuffer[i]));
            auto shadowViewFromWorld = matViewFromWorld(lights[i].view);
            auto shadowTexFromView = mul(shadowTexFromClip, shadowClipFromView, shadowViewFromWorld);

            // Set light values
            lightingBlock->set(buffer, prefix + "color", lights[i].color);
            lightingBlock->set(buffer, prefix + "position", lights[i].view.pose.position);
            lightingBlock->set(buffer, prefix + "matrix", shadowTexFromView);
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
        transformBlock->set(tbuffer, "matClipFromModel", mul(clipFromView, viewFromWorld, obj.pose.matrix()));
        transformBlock->set(tbuffer, "matWorldFromModel", obj.pose.matrix());
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