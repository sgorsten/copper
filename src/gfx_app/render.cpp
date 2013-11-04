#include "render.h"

const char * g_vertShaderPreamble = R"(
    #version 330
    layout(shared, binding = 0) uniform Transform
    {
        mat4 matClipFromModel;
        mat4 matViewFromModel;
    };
)";

const char * g_fragShaderPreamble = R"(
    #version 330
    layout(shared, binding = 4) uniform Lighting
    {
        vec3 ambient;
        vec3 lightPos;
        mat4 lightMatrix;
    };
    layout(binding = 8) uniform sampler2DShadow	u_texShadow;

    vec3 computeLighting(vec3 vsPosition, vec3 vsNormal)
    {
        vec3 eyeDir     = normalize(-vsPosition);
        vec3 light      = ambient;

        vec4 lsPos      = mul(lightMatrix,vec4(vsPosition,1));
        vec3 lightColor = vec3(1,1,1) * shadow2DProj(u_texShadow, lsPos).r;

        vec3 lightDir   = normalize(lightPos - vsPosition);
        vec3 halfDir    = normalize(lightDir + eyeDir);

        light += lightColor * 0.8 * max(dot(lightDir, vsNormal), 0);
        light += lightColor * pow(max(dot(halfDir, vsNormal), 0), 256);

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

void Renderer::render(GlFramebuffer & screen, const std::vector<Object> & objs, const Pose & camPose)
{
    shadowBuffer.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderScene(objs, objs[1].pose, 1.0f, true);

    screen.bind();
    glClearColor(0.2f, 0.6f, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderScene(objs, camPose, (float)screen.dimensions().x/screen.dimensions().y, false);
}

void Renderer::renderScene(const std::vector<Object> & objs, const Pose & camPose, float aspect, bool renderDepth)
{
    auto lightPose = objs[1].pose;

    auto clipFromView = perspectiveMatrixRhGl(1.0f, aspect, 0.1f, 16.0f);
    auto viewFromWorld = camPose.inverse().matrix();

    auto shadowClipFromView = perspectiveMatrixRhGl(1.0f, 1.0f, 0.1f, 16.0f);
    auto shadowViewFromWorld = lightPose.inverse().matrix();

    // Compute matrix that goes from view space to biased shadow clip space
    auto shadowTexFromClip = float4x4{ { 0.5f, 0, 0, 0 }, { 0, 0.5f, 0, 0 }, { 0, 0, 0.5f, 0 }, { 0.5f, 0.5f, 0.5f, 1 } };
    auto worldFromView = camPose.matrix();
    auto shadowTexFromView = mul(shadowTexFromClip, shadowClipFromView, shadowViewFromWorld, worldFromView);

    // Set up lighting UBO
    std::vector<uint8_t> buffer(lightingBlock->pack.size);
    lightingBlock->set(buffer, "ambient", float3(0.2, 0.2, 0.2));
    lightingBlock->set(buffer, "lightPos", transformCoord(viewFromWorld, lightPose.position));
    lightingBlock->set(buffer, "lightMatrix", shadowTexFromView);
    lightingUbo.setData(buffer, GL_STREAM_DRAW);

    std::vector<uint8_t> tbuffer(transformBlock->pack.size);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    if (!renderDepth) glCullFace(GL_BACK);
    else glCullFace(GL_FRONT);

    for (auto & obj : objs)
    {
        auto viewFromModel = mul(viewFromWorld, obj.pose.matrix());

        const auto & prog = renderDepth ? *obj.mat.shadowProg : *obj.mat.prog;

        transformBlock->set(tbuffer, "matClipFromModel", mul(clipFromView, viewFromModel));
        transformBlock->set(tbuffer, "matViewFromModel", viewFromModel);
        transformUbo.setData(tbuffer, GL_DYNAMIC_DRAW);

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