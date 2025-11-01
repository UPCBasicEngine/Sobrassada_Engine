#include "RenderPass.h"
#include "Application.h"
#include "BatchManager.h"
#include "BillboardModule.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "LightsConfig.h"
#include "OpenGLModule.h"
#include "ParticleSystemModule.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "SSAO.h"
#include "ShaderModule.h"
#include "ShaderScriptModule.h"
#include "Standalone/DecalComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/TrailComponent.h"
#include "Standalone/VideoComponent.h"

#ifdef OPTICK
#include "optick.h"
#endif

#include "EngineTimer.h"
#include "WindConfig.h"

#include "Math/Quat.h"
#include <glew.h>

RenderPass::RenderPass()
{
    opaqueMeshesToRender.reserve(1000);
    transparentMeshesToRender.reserve(200);
    vertexOffsetMeshesToRender.reserve(200);
    spotToRender.reserve(TotalShadowMaps);

    glGenBuffers(2, depthReadPBO);
    for (int i = 0; i < 2; ++i)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, depthReadPBO[i]);
        glBufferStorage(
            GL_PIXEL_PACK_BUFFER, 4 * sizeof(float), nullptr,
            GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
        );
        mappedPBO[i] = (float*)glMapBufferRange(
            GL_PIXEL_PACK_BUFFER, 0, 4 * sizeof(float), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
        );
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowResolution, shadowResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        GLOG("Error: Shadow framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SpotLigth shadow map creation
    glGenTextures(TotalShadowMaps, &spotShadowMaps[0]);

    for (int i = 0; i < TotalShadowMaps; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, spotShadowMaps[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SpotLightShadowMapSize, SpotLightShadowMapSize, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        spotShadowMapsGPU[i] = glGetTextureHandleARB(spotShadowMaps[i]);
        glMakeTextureHandleResidentARB(spotShadowMapsGPU[i]);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenBuffers(1, &spotShadowSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotShadowSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SpotlightShadow) * TotalShadowMaps, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    constexpr float cubeVertices[] = {-0.5f, -0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, -0.5f, 0.5f,
                                      -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f};

    constexpr unsigned int cubeIndices[] = {0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7, 4, 5, 1, 1, 0, 4,
                                            3, 2, 6, 6, 7, 3, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};

    glGenVertexArrays(1, &decalVAO);
    glGenBuffers(1, &decalVBO);
    glGenBuffers(1, &decalEBO);

    glBindVertexArray(decalVAO);

    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, decalEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

RenderPass::~RenderPass()
{
    for (int i = 0; i < 2; ++i)
    {
        if (mappedPBO[i])
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, depthReadPBO[i]);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            mappedPBO[i] = nullptr;
        }
    }
    glDeleteBuffers(2, depthReadPBO);

    glDeleteBuffers(1, &visibleLightIndicesSSBO);
    glDeleteBuffers(2, depthReadPBO);
    // glDeleteBuffers(1, &visibleVolumetricAreaIndicesSSBO);

    glDeleteBuffers(1, &decalVBO);
    glDeleteBuffers(1, &decalEBO);
    glDeleteVertexArrays(1, &decalVAO);

    glDeleteTextures(1, &depthTexture);
    glDeleteFramebuffers(1, &depthFBO);

    glDeleteTextures(1, &fogResultTexture);
    glDeleteFramebuffers(2, &blurrFBO[0]);
    glDeleteTextures(2, &blurrTextures[0]);

    for (int i = 0; i < TotalShadowMaps; ++i)
    {
        glMakeTextureHandleNonResidentARB(spotShadowMapsGPU[i]);
    }

    glDeleteTextures(TotalShadowMaps, &spotShadowMaps[0]);

    if (noiseTexture) App->GetResourcesModule()->ReleaseResource(noiseTexture);

    gbuffer     = nullptr;
    framebuffer = nullptr;
}

void RenderPass::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("stepSize", stepSize, allocator);
    targetState.AddMember("fogIntensity", fogIntensity, allocator);
    targetState.AddMember("noiseAmmount", noiseAmmount, allocator);
    targetState.AddMember("extinctionCoefficient", extinctionCoefficient, allocator);
    targetState.AddMember("blurrPasses", blurrPasses, allocator);
    targetState.AddMember("useNoiseTexture", useNoiseTexture, allocator);
    targetState.AddMember("bloomEnabled", bloomEnabled, allocator);
    targetState.AddMember("bloomIntensity", bloomIntensity, allocator);
    targetState.AddMember("noiseTexture", noiseTexture != nullptr ? noiseTexture->GetUID() : INVALID_UID, allocator);
}

void RenderPass::LoadData(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("stepSize")) stepSize = initialState["stepSize"].GetFloat();
    if (initialState.HasMember("fogIntensity")) fogIntensity = initialState["fogIntensity"].GetFloat();
    if (initialState.HasMember("noiseAmmount")) noiseAmmount = initialState["noiseAmmount"].GetFloat();
    if (initialState.HasMember("extinctionCoefficient"))
        extinctionCoefficient = initialState["extinctionCoefficient"].GetFloat();
    if (initialState.HasMember("blurrPasses")) blurrPasses = initialState["blurrPasses"].GetInt();
    if (initialState.HasMember("useNoiseTexture")) useNoiseTexture = initialState["useNoiseTexture"].GetBool();
    if (initialState.HasMember("bloomEnabled")) bloomEnabled = initialState["bloomEnabled"].GetBool();
    if (initialState.HasMember("bloomIntensity")) bloomIntensity = initialState["bloomIntensity"].GetFloat();

    if (initialState.HasMember("noiseTexture"))
    {
        UID textureUID = initialState["noiseTexture"].GetUint64();
        UpdateVolumetricNoiseTexture(textureUID);
    }
}

void RenderPass::Bind() const
{
    framebuffer->Bind();
    glViewport(0, 0, width, height);
}

// Copy Depth to Framebuffer
void RenderPass::CopyDepth() const
{

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer

    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
}

// Copy Depth and Stencil to Framebuffer
void RenderPass::CopyDepthStencil() const
{

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer

    glBlitFramebuffer(
        0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST
    );

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
}

void RenderPass::RenderScene(
    Framebuffer* framebuff, const std::vector<GameObject*> objectsToRender, CameraComponent* camera, float deltaTime
)
{
    ssao         = App->GetOpenGLModule()->GetSsao();
    gbuffer      = App->GetOpenGLModule()->GetGBuffer();
    batchManager = App->GetResourcesModule()->GetBatchManager();
    framebuffer  = framebuff;
    width        = framebuffer->GetTextureWidth();
    height       = framebuffer->GetTextureHeight();
    glViewport(0, 0, width, height);

    glEnable(GL_STENCIL_TEST);

    opaqueMeshesToRender.clear();
    transparentMeshesToRender.clear();
    vertexOffsetMeshesToRender.clear();
    videosToRender.clear();
    groupedDecals.clear();
    shadersToRender.clear();
    trailsToRender.clear();
    spotToRender.clear();

    for (const auto& gameObject : objectsToRender)
    {
        // Meshes
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) && mesh->GetBatch() != nullptr &&
            mesh->GetRenderMode() != 1)
            opaqueMeshesToRender.push_back(mesh);

        else if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) &&
                 mesh->GetBatch() != nullptr && mesh->GetRenderMode() == 1)
        {
            if (mesh->GetResourceMaterial() != nullptr && mesh->GetResourceMaterial()->DoApplyWind())
                vertexOffsetMeshesToRender.push_back(mesh);
            else transparentMeshesToRender.push_back(mesh);
        }

        // Videos
        VideoComponent* video = gameObject->GetComponent<VideoComponent*>();
        if (video != nullptr && video->IsEffectivelyEnabled() && video->IsPlaying()) videosToRender.push_back(video);

        // Trails
        TrailComponent* trail = gameObject->GetComponent<TrailComponent*>();
        if (trail != nullptr && trail->GetEnabled()) trailsToRender.push_back(trail);

        // Shader Scripts
        ShaderScriptComponent* shaderScript = gameObject->GetComponent<ShaderScriptComponent*>();
        if (shaderScript != nullptr) shadersToRender.insert(shaderScript);

        // Decals
        DecalComponent* decal = gameObject->GetComponent<DecalComponent*>();

        if (decal != nullptr)
        {
            if (decal->GetResourceMaterial() != nullptr && decal->IsEffectivelyEnabled())
            {
                const UID uid = decal->GetResourceMaterial()->GetUID();
                groupedDecals[uid].push_back(decal);
            }
        }

        //Spot Lights
        SpotLightComponent* spot = gameObject->GetComponent<SpotLightComponent*>();
        if (spot && spot->GetRenderVolumetric()) spotToRender.push_back(spot);
    }

    if (videosToRender.size() != 0)
    {
#ifdef OPTICK
        OPTICK_PUSH("RenderPass::Video Render")
#endif
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Video Pass");
        gbuffer->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        glDisable(GL_BLEND);

        for (const auto& video : videosToRender)
        {
            video->Render(0.0f, camera);
        }

        glEnable(GL_BLEND);

        gbuffer->Unbind();

#ifdef GAME

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        const unsigned int program = App->GetShaderModule()->GetQuadProgram();
        glUseProgram(program);

        unsigned int loc = glGetUniformLocation(program, "u_Texture");
        glUniform1i(loc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
#else

        Bind();
        const unsigned int program = App->GetShaderModule()->GetQuadProgram();
        glUseProgram(program);
        unsigned int loc = glGetUniformLocation(program, "u_Texture");
        glUniform1i(loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
#endif

        glPopDebugGroup();
#ifdef OPTICK
        OPTICK_POP();
#endif
        return;
    }

#ifdef OPTICK
    OPTICK_PUSH("RenderPass::Geometry PASS")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Geometry Pass");
    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_NAVMESH_MESHES)))
        NavMeshPassRender(objectsToRender, camera);
    else GeometryPassRender(camera);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::Geometry Shaders")
#endif

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Geometry Custom Shaders Pass");
    App->GetShaderScriptModule()->RenderGeometryPassShaders(0.f, camera, shadersToRender);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::ShadowMap")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "ShadowMap Pass");
    DirectionalLightComponent* light = App->GetSceneModule()->GetScene()->GetLightsConfig()->GetDirectionalLight();
    ShadowMapPassRender(camera, light, objectsToRender);
    glPopDebugGroup();

    glViewport(0, 0, width, height);

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::Decals")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Decals Pass");
    DecalsPassRender(camera);
    glPopDebugGroup();

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_GBUFFERS)))
    {
        RenderGBufferDebug(gbuffer);
        return;
    }
    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_DEPTH)))
    {
        RenderDepthDebug(gbuffer, camera);
        return;
    }
    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_SHADOWMAP)) &&
             light != nullptr)
    {
        RenderShadowMapDebug();
        return;
    }
    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_SSAO)))
    {
        RenderSsaoDebug(ssao, camera, framebuffer);
        return;
    }

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::SSAO PASS")
#endif

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SSAO Pass");
    SsaoPassRender(camera, gbuffer, ssao);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SSAO Blur Pass");
    SsaoBlurPassRender(ssao);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::Tile Compute")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Tile Shading");
    TileShadingPass(camera, gbuffer, framebuffer);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::LightPass")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Lighting Pass");
    LightingPassRender(camera, gbuffer, framebuffer);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Height fog Pass");
    HeightFogPassRender(camera);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::Render_TransparentPass")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Transparent Pass");
    TransparentPassRender(objectsToRender, camera);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Transparent Custom Shader Pass");
    App->GetShaderScriptModule()->RenderTransparentPassShaders(0.f, camera, shadersToRender);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::VolumetricRender")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Volumetric Fog Pass");
    VolumetricFogPassRender(camera, light);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::PostLightingShaders")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Post Lighting Custom Shaders Pass");
    App->GetShaderScriptModule()->RenderPostLightingPassShaders(deltaTime, camera, shadersToRender);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::GameObject::Render_Billboards")
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Billboard Pass");
    glEnable(GL_BLEND);
    App->GetBillboardModule()->RenderBillboards();
    glDisable(GL_BLEND);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Particles Pass");
    App->GetParticleModule()->RenderParticles();
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Post effects Pass");
    App->GetShaderScriptModule()->RenderPostEffectsPassShaders(deltaTime, camera, shadersToRender);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Bloom Pass");
    BloomPassRender();
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "FXAA Antialiasing Pass");
    AntiAliasingPassRender(framebuffer);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_POP();
#endif

    batchManager->SwapBuffers();
}

void RenderPass::GeometryPassRender(CameraComponent* camera) const
{
    gbuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_WIREFRAME)))
    {
        App->GetOpenGLModule()->SetRenderWireframe(true);
        batchManager->Render(opaqueMeshesToRender, camera, true);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }
    else batchManager->Render(opaqueMeshesToRender, camera, false);

    glEnable(GL_BLEND);

    gbuffer->Unbind();
}

void RenderPass::NavMeshPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    gbuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    std::vector<MeshComponent*> navMeshesToRender;
    std::vector<MeshComponent*> nonNavMeshesToRender;

    for (const auto& gameObject : objectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) && mesh->GetBatch() != nullptr)
        {
            if (gameObject->IsNavMeshValid()) navMeshesToRender.push_back(mesh);
            else nonNavMeshesToRender.push_back(mesh);
        }
    }

    batchManager->Render(navMeshesToRender, camera, false);
    App->GetOpenGLModule()->SetRenderWireframe(true);
    batchManager->Render(nonNavMeshesToRender, camera, true);
    App->GetOpenGLModule()->SetRenderWireframe(false);

    gbuffer->Unbind();

    glEnable(GL_BLEND);
}

void RenderPass::DepthReduction(unsigned int depthTexture, int gBufferwidth, int gBufferheight)
{
    static struct
    {
        GLint inSize     = -1;
        bool initialized = false;
    } uniforms;

    if (lastReductionSize[0] != gBufferwidth && lastReductionSize[1] != gBufferheight)
    {
        int w = gBufferwidth;
        int h = gBufferheight;

        for (unsigned int tex : reductionTextures)
        {
            glDeleteTextures(1, &tex);
        }
        reductionSizes.clear();
        reductionTextures.clear();

        while (w > 1 || h > 1)
        {
            int groupsX = (w + 7) / 8;
            int groupsY = (h + 3) / 4;

            unsigned int tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, groupsX, groupsY);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            reductionTextures.push_back(tex);
            float2 reductionSize;
            reductionSize.x = static_cast<float>(groupsX);
            reductionSize.y = static_cast<float>(groupsY);
            reductionSizes.push_back(reductionSize);

            w = groupsX;
            h = groupsY;
        }

        lastReductionSize[0] = gBufferwidth;
        lastReductionSize[1] = gBufferheight;
    }

    // Compute shader to find min/max values
    unsigned int currentInput          = depthTexture;

    int currentWidth                   = gBufferwidth;
    int currentHeight                  = gBufferheight;

    unsigned int depthReductionProgram = App->GetShaderModule()->GetComputeShadowDepthProgram();
    glUseProgram(depthReductionProgram);

    if (!uniforms.initialized)
    {
        uniforms.inSize      = glGetUniformLocation(depthReductionProgram, "inSize");
        uniforms.initialized = true;
    }

    for (size_t i = 0; i < reductionTextures.size(); ++i)
    {
        unsigned int currentOutput = reductionTextures[i];
        int groupsX                = reductionSizes[i].x;
        int groupsY                = reductionSizes[i].y;

        glBindImageTexture(0, currentOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindTextureUnit(0, currentInput);
        glUniform2i(uniforms.inSize, currentWidth, currentHeight);

        glDispatchCompute(groupsX, groupsY, 1);
        // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        currentInput  = currentOutput;
        currentWidth  = groupsX;
        currentHeight = groupsY;
    }

    int writePBOIndex = 1 - currentPBOIndex;

    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, depthReadPBO[writePBOIndex]);
    glBindTexture(GL_TEXTURE_2D, currentInput);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, 0); // offset 0 en el PBO
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    currentPBOIndex = writePBOIndex;
}

void RenderPass::ShadowMapPassRender(
    CameraComponent* camera, DirectionalLightComponent* light, const std::vector<GameObject*>& objectsToRender
)
{
    if (light == nullptr) return;

    glEnable(GL_DEPTH_CLAMP);

#ifdef OPTICK
    OPTICK_PUSH("RenderPass::ShadowMap::Directional")
#endif
    int readPBOIndex = 1 - currentPBOIndex;

    if (mappedPBO[readPBOIndex])
    {
        lastFrameMinDepth = mappedPBO[readPBOIndex][0];
        lastFrameMaxDepth = mappedPBO[readPBOIndex][1];
    }

    // glBindBuffer(GL_PIXEL_PACK_BUFFER, depthReadPBO[readPBOIndex]);
    // float* ptr = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    // if (ptr)
    //{
    //     lastFrameMinDepth = ptr[0];
    //     lastFrameMaxDepth = ptr[1];
    //     glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    // }
    // glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

#ifdef OPTICK
    OPTICK_PUSH("RenderPass::ShadowMap::DepthReduction");
#endif
    DepthReduction(gbuffer->GetDepthTexture(), gbuffer->GetScreenWidth(), gbuffer->GetScreenHeight());
#ifdef OPTICK
    OPTICK_POP();
#endif

    float minDepth = lastFrameMinDepth;
    float maxDepth = lastFrameMaxDepth;

    // Compute the near and far planes based on the min/max depth values
    float nearD;
    float farD;
    camera == nullptr ? nearD = App->GetCameraModule()->GetNearPlaneDistance() : nearD = camera->GetNearPlaneDistance();
    camera == nullptr ? farD = App->GetCameraModule()->GetFarPlaneDistance() : farD = camera->GetFarPlaneDistance();

    float S       = (-2 * farD * nearD) / (farD - nearD);
    float T       = -(nearD + farD) / (farD - nearD);

    float distMin = S / (T + minDepth);
    float distMax = S / (T + maxDepth);

    // GLOG("Final reduction size: %d, %d", currentWidth, currentHeight);
    // GLOG("%f, %f", distMin, distMax);

    camera == nullptr ? App->GetCameraModule()->SetNear(distMin) : camera->SetNear(distMin);
    camera == nullptr ? App->GetCameraModule()->SetFar(distMax) : camera->SetFar(distMax);

    // Compute light
    float3 corners[8];
    camera == nullptr ? App->GetCameraModule()->GetFrustumCorners(corners) : camera->GetFrustumCorners(corners);

    float3 sphereCenter = float3::zero;

    camera == nullptr ? sphereCenter = App->GetCameraModule()->GetCamera().CenterPoint()
                      : sphereCenter = camera->GetCameraCenter();

    float sphereRadius = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        float dist = (corners[i] - sphereCenter).Length();
        if (dist > sphereRadius) sphereRadius = dist;
    }

    float3 lightDir = light->GetDirection();
    lightDir.Normalize();

    float3 worldUp = float3::unitY;
    if (fabs(lightDir.Dot(worldUp)) > 0.99f) worldUp = float3(1.0f, 0.0f, 0.0f);

    float3 lightRight = worldUp.Cross(lightDir);
    lightRight.Normalize();
    float3 lightUp = lightDir.Cross(lightRight);
    lightUp.Normalize();

    Frustum shadowfrustum;

    shadowfrustum.type               = FrustumType::OrthographicFrustum;
    shadowfrustum.pos                = sphereCenter + lightDir * sphereRadius;
    shadowfrustum.front              = lightDir;
    shadowfrustum.up                 = lightUp;
    shadowfrustum.orthographicWidth  = sphereRadius * 2.3f;
    shadowfrustum.orthographicHeight = sphereRadius * 2.3f;
    shadowfrustum.nearPlaneDistance  = 0.001f;
    shadowfrustum.farPlaneDistance   = sphereRadius * 2.3f;

    CameraMatrices lightmatrices;
    lightmatrices.viewMatrix       = shadowfrustum.ViewMatrix();
    lightmatrices.projectionMatrix = shadowfrustum.ProjectionMatrix();
    lightView                      = shadowfrustum.ViewMatrix();
    lightProj                      = shadowfrustum.ProjectionMatrix();

    // DebugDrawModule* debugdraw     = App->GetDebugDrawModule();
    // debugdraw->DrawFrustrum(lightProj, lightView);

    unsigned int ubo               = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatrices), &lightmatrices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glViewport(0, 0, shadowResolution, shadowResolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    std::vector<MeshComponent*> meshesToRender;

    FrustumPlanes lightFrustum;
    lightFrustum.UpdateFrustumPlanes(lightView, lightProj);

    std::vector<GameObject*> shadowObjectsToRender;
    App->GetSceneModule()->GetScene()->CheckObjectsInFrustum(shadowObjectsToRender, lightFrustum);

    for (const auto& gameObject : shadowObjectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) && mesh->GetBatch() != nullptr &&
            mesh->GetRenderMode() != 1 && mesh->GetProduceShadows())
            meshesToRender.push_back(mesh);
    }

    batchManager->RenderShadowMap(meshesToRender, ubo);

    camera == nullptr ? App->GetCameraModule()->SetNear(nearD) : camera->SetNear(nearD);
    camera == nullptr ? App->GetCameraModule()->SetFar(farD) : camera->SetFar(farD);

    // RENDER SPOTLIGHT SHADOWMAPS
#ifdef OPTICK
    OPTICK_POP();
    OPTICK_PUSH("RenderPass::ShadowMap::Spotlights")
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glViewport(0, 0, SpotLightShadowMapSize, SpotLightShadowMapSize);
    std::vector<GameObject*> shadowObjectsToRenderSpot;

    for (int i = 0; i < TotalShadowMaps && i < spotToRender.size(); ++i)
    {
        if (!spotToRender[i] || (spotToRender[i] && !spotToRender[i]->GetRenderVolumetric())) continue;

        meshesToRender.clear();
        shadowObjectsToRenderSpot.clear();

        lightFrustum.UpdateFrustumPlanes(spotToRender[i]->GetViewMatrix(), spotToRender[i]->GetProjectionMatrix());
        App->GetSceneModule()->GetScene()->CheckObjectsInFrustum_Cached(
            shadowObjectsToRenderSpot, lightFrustum, shadowObjectsToRender
        );

        for (const auto& gameObject : shadowObjectsToRenderSpot)
        {
            MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
            if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) &&
                mesh->GetBatch() != nullptr && mesh->GetRenderMode() != 1 && mesh->GetProduceShadows())
                meshesToRender.push_back(mesh);
        }

        if (meshesToRender.size() < 1) continue;

        glBindTexture(GL_TEXTURE_2D, spotShadowMaps[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spotShadowMaps[i], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        lightmatrices.viewMatrix       = spotToRender[i]->GetViewMatrix();
        lightmatrices.projectionMatrix = spotToRender[i]->GetProjectionMatrix();

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatrices), &lightmatrices, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // LOADING SPOTLIGHT SHADOW TO SSBO
        spotToRender[i]->SetShadowGPUIndex(i);
        SpotlightShadow currentShadow;
        currentShadow.viewProjection = spotToRender[i]->GetViewProjection().Transposed();
        currentShadow.shadowMap      = spotShadowMapsGPU[i];

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotShadowSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(SpotlightShadow) * i, sizeof(SpotlightShadow), &currentShadow);

        batchManager->RenderShadowMap(meshesToRender, ubo);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glDeleteBuffers(1, &ubo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_CLAMP);

#ifdef OPTICK
    OPTICK_POP();
#endif
}

void RenderPass::SsaoPassRender(CameraComponent* camera, GBuffer* gbuffer, SSAO* ssao) const
{
    static struct
    {
        GLint gPositions     = -1;
        GLint gNormals       = -1;
        GLint gDepth         = -1;
        GLint noiseTexture   = -1;
        GLint kernel_samples = -1;
        GLint screenSize     = -1;
        GLint bias           = -1;
        GLint range          = -1;
        GLuint cameraBlock   = GL_INVALID_INDEX;
        bool initialized     = false;
    } uniforms;

    ssao->Bind();
    const unsigned int program = App->GetShaderModule()->GetSsaoProgram();

    glViewport(0, 0, ssao->GetWidth(), ssao->GetHeight());
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    if (!uniforms.initialized)
    {
        uniforms.gPositions     = glGetUniformLocation(program, "gPositions");
        uniforms.gNormals       = glGetUniformLocation(program, "gNormals");
        uniforms.gDepth         = glGetUniformLocation(program, "gDepth");
        uniforms.noiseTexture   = glGetUniformLocation(program, "noiseTexture");
        uniforms.kernel_samples = glGetUniformLocation(program, "kernel_samples");
        uniforms.screenSize     = glGetUniformLocation(program, "screenSize");
        uniforms.bias           = glGetUniformLocation(program, "bias");
        uniforms.range          = glGetUniformLocation(program, "range");
        uniforms.cameraBlock    = glGetUniformBlockIndex(program, "CameraMatrices");
        uniforms.initialized    = true;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);
    glUniform1i(uniforms.gPositions, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);
    glUniform1i(uniforms.gNormals, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->GetDepthTexture());
    glUniform1i(uniforms.gDepth, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ssao->GetNoiseTexture());
    glUniform1i(uniforms.noiseTexture, 3);

    glUniform3fv(uniforms.kernel_samples, SSAO_KERNEL_SIZE_MID, &ssao->GetKernels()[0].x);

    glUniform2f(uniforms.screenSize, (float)ssao->GetWidth(), (float)ssao->GetHeight());
    glUniform1f(uniforms.bias, 0.025f);
    glUniform1f(uniforms.range, 0.5f);
    unsigned int cameraUBO;
    if (camera == nullptr)
    {
        cameraUBO = App->GetCameraModule()->GetUbo();
    }
    else
    {
        cameraUBO = camera->GetUbo();
    }

    glUniformBlockBinding(program, uniforms.cameraBlock, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);

    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);

    ssao->Unbind();
}

void RenderPass::SsaoBlurPassRender(SSAO* ssao)
{
    static struct
    {
        GLint horizontal = -1;
        GLint ssaoInput  = -1;
        bool initialized = false;
    } uniforms;

    const GLuint blurShader = App->GetShaderModule()->GetSsaoBlurProgram();

    glUseProgram(blurShader);

    if (!uniforms.initialized)
    {
        uniforms.horizontal  = glGetUniformLocation(blurShader, "horizontal");
        uniforms.ssaoInput   = glGetUniformLocation(blurShader, "ssaoInput");
        uniforms.initialized = true;
    }

    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ssao->GetBlurFBO(i));
        glViewport(0, 0, ssao->GetWidth(), ssao->GetHeight());
        glClear(GL_COLOR_BUFFER_BIT);

        bool horizontal = (i == 0);

        glUniform1i(uniforms.horizontal, horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, i == 0 ? ssao->GetSSAOTexture() : ssao->GetBlurTexture(0));
        glUniform1i(uniforms.ssaoInput, 0);

        App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass::HeightFogPassRender(CameraComponent* camera) const
{
    if (!heightFog.isEnabled) return;

    Bind();

    const unsigned int program = App->GetShaderModule()->GetHeightFogProgram();
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer->GetDepthTexture());

    glUniform1f(2, heightFog.densityConstant);
    glUniform1f(3, heightFog.heightFalloff);

    const float4x4 cameraMatrix = camera ? camera->GetWorldMatrix() : App->GetCameraModule()->GetWorldMatrix();
    const float3 cameraPos      = camera ? camera->GetCameraPosition() : App->GetCameraModule()->GetCameraPosition();
    const float4x4 projection = camera ? camera->GetProjectionMatrix() : App->GetCameraModule()->GetProjectionMatrix();
    glUniform3fv(4, 1, cameraPos.ptr());
    glUniformMatrix4fv(5, 1, GL_TRUE, cameraMatrix.ptr());
    glUniformMatrix4fv(6, 1, GL_TRUE, projection.ptr());

    glUniform1f(7, heightFog.maxFog);
    glUniform3fv(8, 1, heightFog.fogColor.ptr());
    glUniform1f(9, heightFog.fogStartHeight);
    glUniform1i(10, heightFog.followCamera);

    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);

    glDepthMask(GL_TRUE);
}

void RenderPass::BloomPassRender() const
{
    if (!bloomEnabled) return;

    bool horizontal = true, firstIteration = true;

    const unsigned int blurrProgram = App->GetShaderModule()->GetGaussianBlurrProgram();
    glUseProgram(blurrProgram);
    glViewport(0, 0, width / 2, height / 2);

    for (int i = 0; i < 6; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurrFBO[horizontal]);
        glUniform1ui(0, horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? gbuffer->emissiveTexture : blurrTextures[!horizontal]);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        horizontal = !horizontal;
        if (firstIteration) firstIteration = false;
    }

    Bind();

    const unsigned int bloomProgram = App->GetShaderModule()->GetBloomProgram();
    glUseProgram(bloomProgram);

    static struct
    {
        GLint bloomIntensity = -1;
        bool initialized     = false;
    } uniforms;

    if (!uniforms.initialized)
    {
        uniforms.bloomIntensity = glGetUniformLocation(bloomProgram, "bloomIntensity");
        uniforms.initialized    = true;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer->GetColorTexture());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurrTextures[!horizontal]);

    glUniform1f(uniforms.bloomIntensity, bloomIntensity);

    glDepthMask(GL_FALSE);
    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
}

void RenderPass::AntiAliasingPassRender(Framebuffer* framebuffer) const
{
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint fxaaTexture = -1;

#ifndef GAME
    if (!fxaaParameters.isEnabled) return;

    // Must create a temporal frameBuffer and texture, to avoid reading and drawing to same texture = black screen
    GLuint fxaaFramebuffer = -1;
    glGenFramebuffers(1, &fxaaFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fxaaFramebuffer);

    glGenTextures(1, &fxaaTexture);
    glBindTexture(GL_TEXTURE_2D, fxaaTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fxaaTexture, 0);

    // Copy
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->GetFramebufferID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fxaaFramebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    Bind();

#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    fxaaTexture = framebuffer->GetColorTexture();
#endif

    unsigned int fxaaProgram = App->GetShaderModule()->GetFXAAProgram();
    glUseProgram(fxaaProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fxaaTexture);

    glUniform1i(0, fxaaParameters.showBorders);
    glUniform1f(1, fxaaParameters.globalThreshold);
    glUniform1f(2, fxaaParameters.localThreshold);
    glUniform1i(3, fxaaParameters.isEnabled);

    glDepthMask(GL_FALSE);
    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);

#ifndef GAME
    glDeleteFramebuffers(1, &fxaaFramebuffer);
    glDeleteTextures(1, &fxaaTexture);
#endif
}

void RenderPass::VolumetricFogPassRender(CameraComponent* camera, DirectionalLightComponent* light)
{
#ifdef OPTICK
    OPTICK_CATEGORY("RenderPass::VolumetricFog", Optick::Category::Rendering)
#endif
    if (!light) return;

    if (fogResultTexture == 0)
    {
        glGenTextures(1, &fogResultTexture);

        glBindTexture(GL_TEXTURE_2D, fogResultTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    if (blurrFBO[0] == 0)
    {
        // VOLUMETRIC GAUSS BLURR
        glCreateFramebuffers(2, &blurrFBO[0]);
        glGenTextures(2, &blurrTextures[0]);

        for (unsigned int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, blurrFBO[i]);
            glBindTexture(GL_TEXTURE_2D, blurrTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurrTextures[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                GLOG("ERROR::VolumetricFog::Framebuffer %i is not complete!\n", i);
            }
        }
    }

    glUseProgram(App->GetShaderModule()->GetVolumetricFogComputeProgram());

    glBindImageTexture(0, fogResultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glBindTextureUnit(1, framebuffer->GetDepthTexture());
    glBindTextureUnit(2, depthTexture);

    if (useNoiseTexture && noiseTexture) glBindTextureUnit(3, noiseTexture->GetTextureID());

    LightsConfig* lConfig = App->GetSceneModule()->GetScene()->GetLightsConfig();
    lConfig->SetLightsShaderData();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, visibleLightIndicesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, spotShadowSSBO);
    lConfig->SetVolumetricAreaShaderData(); // 8 binding spot
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, visibleVolumetricAreaIndicesSSBO);

    // Local size of compute is (16,16,1)
    unsigned int numGroupsX = (width / 2 + (8 - 1)) / 8;
    unsigned int numGroupsY = (height / 2 + (8 - 1)) / 8;

    float3 cameraPosition;
    float4x4 projection, inverseView;

    if (camera)
    {
        projection     = camera->GetProjectionMatrix();
        inverseView    = camera->GetWorldMatrix();
        cameraPosition = camera->GetCameraPosition();
    }
    else
    {
        projection     = App->GetCameraModule()->GetProjectionMatrix();
        inverseView    = App->GetCameraModule()->GetWorldMatrix();
        cameraPosition = App->GetCameraModule()->GetCameraPosition();
    }

    glUniformMatrix4fv(0, 1, GL_TRUE, &projection[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &inverseView[0][0]);
    glUniform3fv(2, 1, &cameraPosition[0]);

    float time = App->GetGameTimer()->GetTime();

    glUniform1ui(3, useNoiseTexture);
    glUniform1f(4, fogIntensity);
    glUniform1f(5, extinctionCoefficient);
    glUniform1f(6, time);
    glUniform1f(7, noiseAmmount);
    // glUniform1f(8, anisotropy);
    glUniform1i(9, tilesX);
    glUniform1f(10, stepSize);

    // THIS WILL PROBABLY CHANGE WITH CHANGES TO SHADOWS
    // Compute light
    float3 corners[8];
    camera == nullptr ? App->GetCameraModule()->GetFrustumCorners(corners) : camera->GetFrustumCorners(corners);

    float3 sphereCenter = float3::zero;

    camera == nullptr ? sphereCenter = App->GetCameraModule()->GetCamera().CenterPoint()
                      : sphereCenter = camera->GetCameraCenter();
    float sphereRadius = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        float dist = (corners[i] - sphereCenter).Length();
        if (dist > sphereRadius) sphereRadius = dist;
    }

    float3 lightDir = light->GetDirection();
    lightDir.Normalize();

    float3 worldUp = float3::unitY;
    if (fabs(lightDir.Dot(worldUp)) > 0.99f) worldUp = float3(1.0f, 0.0f, 0.0f);

    float3 lightRight = worldUp.Cross(lightDir);
    lightRight.Normalize();
    float3 lightUp = lightDir.Cross(lightRight);
    lightUp.Normalize();

    Frustum shadowfrustum;

    shadowfrustum.type               = FrustumType::OrthographicFrustum;
    shadowfrustum.pos                = sphereCenter + lightDir * sphereRadius;
    shadowfrustum.front              = lightDir;
    shadowfrustum.up                 = lightUp;
    shadowfrustum.orthographicWidth  = sphereRadius * 2.0f;
    shadowfrustum.orthographicHeight = sphereRadius * 2.0f;
    shadowfrustum.nearPlaneDistance  = 0.1f;
    shadowfrustum.farPlaneDistance   = sphereRadius * 2.0f;

    float4x4 dirLightProj, dirLightView;

    dirLightView = shadowfrustum.ViewMatrix();
    dirLightProj = shadowfrustum.ProjectionMatrix();

    glUniformMatrix4fv(11, 1, GL_TRUE, &dirLightView[0][0]);
    glUniformMatrix4fv(12, 1, GL_TRUE, &dirLightProj[0][0]);

    glDispatchCompute(numGroupsX, numGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

    // APPLYING BLURR TO VOLUMETRICS
    glDepthMask(GL_FALSE);

    bool horizontal = true, firstIteration = true;

    unsigned int blurrProgram = App->GetShaderModule()->GetGaussianBlurrProgram();
    glUseProgram(blurrProgram);
    glViewport(0, 0, width / 2, height / 2);

    for (int i = 0; i < blurrPasses; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurrFBO[horizontal]);
        glUniform1ui(0, horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? fogResultTexture : blurrTextures[!horizontal]);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        horizontal = !horizontal;
        if (firstIteration) firstIteration = false;
    }

    // RENDER COMPUTED TEXTURE ON TOP OF SCENE

    Bind();

    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    static struct
    {
        GLint u_Texture  = -1;
        bool initialized = false;
    } uniforms;

    unsigned int quadProgram = App->GetShaderModule()->GetQuadProgram();

    glUseProgram(quadProgram);

    if (!uniforms.initialized)
    {
        uniforms.u_Texture   = glGetUniformLocation(quadProgram, "u_Texture");
        uniforms.initialized = true;
    }

    glUniform1i(uniforms.u_Texture, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurrTextures[!horizontal]);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void RenderPass::DecalsPassRender(CameraComponent* camera) const
{
    if (groupedDecals.empty()) return;

    static struct
    {
        GLint positionTex      = -1;
        GLint normalTex        = -1;
        GLint decalAlbedoTex   = -1;
        GLint hasMetallic      = -1;
        GLint decalMetallicTex = -1;
        GLint hasNormal        = -1;
        GLint decalNormalTex   = -1;
        GLuint cameraBlock     = GL_INVALID_INDEX;
        bool initialized       = false;
    } uniforms;

    gbuffer->Bind();

    const unsigned int program = App->GetShaderModule()->GetDecalProgram();

    glUseProgram(program);

    if (!uniforms.initialized)
    {
        uniforms.positionTex      = glGetUniformLocation(program, "positionTex");
        uniforms.normalTex        = glGetUniformLocation(program, "normalTex");
        uniforms.decalAlbedoTex   = glGetUniformLocation(program, "decalAlbedoTex");
        uniforms.hasMetallic      = glGetUniformLocation(program, "hasMetallic");
        uniforms.decalMetallicTex = glGetUniformLocation(program, "decalMetallicTex");
        uniforms.hasNormal        = glGetUniformLocation(program, "hasNormal");
        uniforms.decalNormalTex   = glGetUniformLocation(program, "decalNormalTex");
        uniforms.cameraBlock      = glGetUniformBlockIndex(program, "CameraMatrices");
        uniforms.initialized      = true;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);
    glUniform1i(uniforms.positionTex, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);
    glUniform1i(uniforms.normalTex, 1);

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    glUniformBlockBinding(program, uniforms.cameraBlock, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);

    for (const auto& [uid, decals] : groupedDecals)
    {

        const uint64_t dhandle = decals[0]->GetResourceMaterial()->GetMaterial().diffuseTex;
        glUniformHandleui64ARB(uniforms.decalAlbedoTex, dhandle);

        if (decals[0]->GetResourceMaterial()->GetMaterial().hasMetallic)
        {
            glUniform1i(uniforms.hasMetallic, 1);

            const uint64_t mhandle = decals[0]->GetResourceMaterial()->GetMaterial().metallicTex;
            glUniformHandleui64ARB(uniforms.decalMetallicTex, mhandle);
        }
        else if (decals[0]->GetResourceMaterial()->GetMaterial().hasSpecular)
        {
            glUniform1i(uniforms.hasMetallic, 1);

            const uint64_t mhandle = decals[0]->GetResourceMaterial()->GetMaterial().specularTex;
            glUniformHandleui64ARB(uniforms.decalMetallicTex, mhandle);
        }
        else glUniform1i(uniforms.hasMetallic, 0);

        glUniform1i(uniforms.hasNormal, decals[0]->GetResourceMaterial()->HasNormal() ? 1 : 0);

        const uint64_t nhandle = decals[0]->GetResourceMaterial()->GetMaterial().normalTex;
        glUniformHandleui64ARB(uniforms.decalNormalTex, nhandle);

        std::vector<DecalModels> models;
        models.reserve(decals.size());

        for (const auto& decal : decals)
        {
            if (!decal->GetEnabled()) continue;
            float4x4 model    = decal->GetParent()->GetGlobalTransform();
            float4x4 invModel = model.Inverted();

            models.push_back({model, invModel});
        }

        if (models.empty()) continue;

        GLuint decalSSBO;
        glGenBuffers(1, &decalSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, decalSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DecalModels) * models.size(), models.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, decalSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glBindVertexArray(decalVAO);

        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, (GLsizei)models.size());

        glBindVertexArray(0);

        glDeleteBuffers(1, &decalSSBO);
    }

    gbuffer->Unbind();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void RenderPass::TileShadingPass(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer)
{
    static struct
    {
        GLint screenSize   = -1;
        GLuint cameraBlock = GL_INVALID_INDEX;
        bool initialized   = false;
    } uniforms;

    const int TILE_SIZE             = 16;
    const int MAX_LIGHTS_PER_TILE   = 250;

    tilesX                          = (width + TILE_SIZE - 1) / TILE_SIZE;
    int tilesY                      = (height + TILE_SIZE - 1) / TILE_SIZE;
    int numTiles                    = tilesX * tilesY;

    int totalIndices                = numTiles * MAX_LIGHTS_PER_TILE;
    size_t totalSize                = numTiles * MAX_LIGHTS_PER_TILE * sizeof(int);

    unsigned int tileShadingProgram = App->GetShaderModule()->GetTileShadingProgram();
    glUseProgram(tileShadingProgram);

    if (!uniforms.initialized)
    {
        uniforms.screenSize  = glGetUniformLocation(tileShadingProgram, "screenSize");
        uniforms.cameraBlock = glGetUniformBlockIndex(tileShadingProgram, "CameraMatrices");
        uniforms.initialized = true;
    }

    glUniform2i(uniforms.screenSize, width, height);

    glBindTextureUnit(0, gbuffer->GetDepthTexture());

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    glUniformBlockBinding(tileShadingProgram, uniforms.cameraBlock, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);

    if (visibleLightIndicesSSBO == 0 || totalSize != currentSize)
    {
        if (visibleLightIndicesSSBO != 0)
        {
            glDeleteBuffers(1, &visibleLightIndicesSSBO);
        }

        glGenBuffers(1, &visibleLightIndicesSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);

        currentSize = totalSize;
    }

    // if (visibleVolumetricAreaIndicesSSBO == 0 || totalSize != currentSize)
    //{
    //     if (visibleVolumetricAreaIndicesSSBO != 0)
    //     {
    //         glDeleteBuffers(1, &visibleVolumetricAreaIndicesSSBO);
    //     }

    //    glGenBuffers(1, &visibleVolumetricAreaIndicesSSBO);
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleVolumetricAreaIndicesSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);
    //}

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesSSBO);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visibleVolumetricAreaIndicesSSBO);

    App->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();
    // App->GetSceneModule()->GetScene()->GetLightsConfig()->SetVolumetricAreaShaderData();

    glDispatchCompute(tilesX, tilesY, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void RenderPass::LightingPassRender(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer) const
{
    Bind();

    glDisable(GL_BLEND);

    // SKYBOX
    if (!App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_WIREFRAME))
    {
        float4x4 projection;
        float4x4 view;

        if (camera == nullptr)
            App->GetSceneModule()->GetScene()->GetLightsConfig()->RenderSkybox(
                App->GetCameraModule()->GetProjectionMatrix(), App->GetCameraModule()->GetViewMatrix()
            );
        else
        {
            bool change = false;
            // Cubemap does not support Ortographic projection
            if (camera->GetFrustumType() == 1)
            {
                change = true;
                camera->ChangeToPerspective();
            }
            App->GetSceneModule()->GetScene()->GetLightsConfig()->RenderSkybox(
                camera->GetProjectionMatrix(), camera->GetViewMatrix()
            );
            if (change) camera->ChangeToOrtographic();
        }
    }

    // COPYING DEPTH BUFFER AND STENCIL FROM GBUFFER TO RENDER FRAMEBUFFER
    // TODO CHECK IF GAME RELEASE TO RENDER TO DEFAULT BUFFER INSTEAD OF FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

    CopyDepthStencil();

    // SETTING STENCIL TEST FOR ONLY RENDER TO GBUFFER FRAGMENTS WRITES
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0xFF);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->specularTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, gbuffer->emissiveTexture);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, ssao->GetBlurTexture(1));

    App->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

    unsigned int lightingPassProgram = App->GetShaderModule()->GetLightingPassProgram();

    glUseProgram(lightingPassProgram);

    static struct
    {
        GLint viewLight      = -1;
        GLint projLight      = -1;
        GLint shadowTint     = -1;
        GLint shadowStrength = -1;
        GLint cameraPos      = -1;
        GLint numTilesX      = -1;
        GLint screenSize     = -1;
        bool initialized     = false;
    } uniforms;

    if (!uniforms.initialized)
    {
        uniforms.viewLight      = glGetUniformLocation(lightingPassProgram, "viewLight");
        uniforms.projLight      = glGetUniformLocation(lightingPassProgram, "projLight");
        uniforms.shadowTint     = glGetUniformLocation(lightingPassProgram, "shadowTint");
        uniforms.shadowStrength = glGetUniformLocation(lightingPassProgram, "shadowStrength");
        uniforms.cameraPos      = glGetUniformLocation(lightingPassProgram, "cameraPos");
        uniforms.numTilesX      = glGetUniformLocation(lightingPassProgram, "numTilesX");
        uniforms.screenSize     = glGetUniformLocation(lightingPassProgram, "screenSize");
        uniforms.initialized    = true;
    }

    float3 cameraPos;
    if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
    else cameraPos = camera->GetCameraPosition();

    glUniformMatrix4fv(uniforms.viewLight, 1, GL_TRUE, lightView.ptr());
    glUniformMatrix4fv(uniforms.projLight, 1, GL_TRUE, lightProj.ptr());

    DirectionalLightComponent* light = App->GetSceneModule()->GetScene()->GetLightsConfig()->GetDirectionalLight();
    if (light != nullptr)
    {
        float3 shadowTint = light->GetShadowTint();
        glUniform3f(uniforms.shadowTint, shadowTint.x, shadowTint.y, shadowTint.z);
        float shadowStrength = light->GetShadowStrength();
        glUniform1f(uniforms.shadowStrength, shadowStrength);
    }

    glUniform3fv(uniforms.cameraPos, 1, &cameraPos[0]);

    // Light Culling
    glUniform1i(uniforms.numTilesX, tilesX);
    glUniform2i(uniforms.screenSize, width, height);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, visibleLightIndicesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, spotShadowSSBO);

    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);

    // COPYING DEPTH BUFFER FROM GBUFFER TO RENDER FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

    CopyDepth();
}

void RenderPass::TransparentPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    static struct
    {
        GLint cameraPos   = -1;
        GLint isWireframe = -1;
        bool initialized  = false;
    } tuniforms;

    static struct
    {
        GLint cameraPos      = -1;
        GLint isWireframe    = -1;
        GLint windDirection  = -1;
        GLint windParameters = -1;
        bool initialized     = false;
    } wtuniforms;

    Bind();

    glDepthMask(GL_FALSE);

    const unsigned int program    = App->GetShaderModule()->GetTransparentPassProgram();
    const unsigned int wPOProgram = App->GetShaderModule()->GetTransparentVPOPassProgram();

    glUseProgram(program);

    if (!tuniforms.initialized)
    {
        tuniforms.cameraPos   = glGetUniformLocation(program, "cameraPos");
        tuniforms.isWireframe = glGetUniformLocation(program, "isWireframe");
        tuniforms.initialized = true;
    }

    App->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

    float3 cameraPos;
    if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
    else cameraPos = camera->GetCameraPosition();

    glUniform3fv(tuniforms.cameraPos, 1, &cameraPos[0]);

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_NAVMESH_MESHES)))
    {
        std::vector<MeshComponent*> navmeshesToRender;
        std::vector<MeshComponent*> nonnavmeshesToRender;

        for (const auto& gameObject : objectsToRender)
        {
            MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
            if (mesh != nullptr && (mesh->GetEnabled() || mesh->GetUpdateShaderStorage()) &&
                mesh->GetBatch() != nullptr && mesh->GetRenderMode() == 1)
            {
                if (gameObject->IsNavMeshValid()) navmeshesToRender.push_back(mesh);
                else nonnavmeshesToRender.push_back(mesh);
            }
        }

        glUniform1i(tuniforms.isWireframe, 0);
        batchManager->RenderTransparent(navmeshesToRender, program, camera);
        glUniform1i(tuniforms.isWireframe, 1);
        App->GetOpenGLModule()->SetRenderWireframe(true);
        batchManager->RenderTransparent(nonnavmeshesToRender, program, camera);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }

    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_WIREFRAME)))
    {
        glUniform1i(tuniforms.isWireframe, 1);
        App->GetOpenGLModule()->SetRenderWireframe(true);
        batchManager->RenderTransparent(transparentMeshesToRender, program, camera);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }

    else
    {
        glUniform1i(tuniforms.isWireframe, 0);

        batchManager->RenderTransparent(transparentMeshesToRender, program, camera);

        glUseProgram(wPOProgram);

        if (!wtuniforms.initialized)
        {
            wtuniforms.cameraPos      = glGetUniformLocation(wPOProgram, "cameraPos");
            wtuniforms.isWireframe    = glGetUniformLocation(wPOProgram, "isWireframe");
            wtuniforms.windDirection  = glGetUniformLocation(wPOProgram, "windDirection");
            wtuniforms.windParameters = glGetUniformLocation(wPOProgram, "windParameters");
            wtuniforms.initialized    = true;
        }

        glUniform3fv(wtuniforms.cameraPos, 1, &cameraPos[0]);
        glUniform1i(wtuniforms.isWireframe, 0);

        WindConfig* windConfig = App->GetSceneModule()->GetScene()->GetWindsConfig();
        if (windConfig->GetApplyWindGlobally() && !vertexOffsetMeshesToRender.empty())
        {
            const Quat windDirection = Quat::FromEulerXYZ(0, windConfig->GetWindDirection() * DEGREE_RAD_CONV, 0);
            glUniform4f(wtuniforms.windDirection, windDirection.x, windDirection.y, windDirection.z, windDirection.w);
            glUniform4f(
                wtuniforms.windParameters, App->GetEngineTimer()->GetTime(), windConfig->GetWindSpeed(),
                std::max(1.f, windConfig->GetGustFrequency()), windConfig->GetGustSpeed()
            );
        }
        batchManager->RenderTransparent(vertexOffsetMeshesToRender, wPOProgram, camera);

        glEnable(GL_BLEND);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        static struct
        {
            GLuint cameraBlock = GL_INVALID_INDEX;
            bool initialized   = false;
        } trailUniforms;

        const unsigned int program = App->GetShaderModule()->GetTrailProgram();
        glUseProgram(program);

        if (!trailUniforms.initialized)
        {
            trailUniforms.cameraBlock = glGetUniformBlockIndex(program, "CameraMatrices");
            trailUniforms.initialized = true;
        }

        unsigned int cameraUBO;
        if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
        else cameraUBO = camera->GetUbo();

        /*glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
        glUniformBlockBinding(program, blockIdx, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);*/

        glUniformBlockBinding(program, trailUniforms.cameraBlock, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);

        for (const auto& trail : trailsToRender)
            trail->Render(0, nullptr);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void RenderPass::RenderGBufferDebug(GBuffer* gbuffer) const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetQuadProgram();
    glUseProgram(program);

    unsigned int loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    // Top-left: Diffuse
    glViewport(0, height / 2, width / 2, height / 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Top-right: Specular
    glViewport(width / 2, height / 2, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->specularTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Bottom-left: Position
    glViewport(0, 0, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Bottom-right: Normal
    glViewport(width / 2, 0, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glViewport(0, 0, width, height);
}

void RenderPass::RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetLinearDepthProgram();
    glUseProgram(program);

    unsigned int loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->GetDepthTexture());

    float nearPlane;
    float farPlane;
    if (camera == nullptr)
    {
        nearPlane = App->GetCameraModule()->GetNearPlaneDistance();
        farPlane  = 100; // Far plane is too much
    }
    else
    {
        nearPlane = camera->GetNearPlaneDistance();
        farPlane  = camera->GetFarPlaneDistance();
    }

    glUniform1f(glGetUniformLocation(program, "nearPlane"), nearPlane);
    glUniform1f(glGetUniformLocation(program, "farPlane"), farPlane);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void RenderPass::RenderSsaoDebug(SSAO* ssao, CameraComponent* camera, Framebuffer* framebuffer) const
{
    framebuffer->Bind();

    glViewport(0, 0, width, height);

    unsigned int program = App->GetShaderModule()->GetSsaoDebugProgram();
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssao->GetBlurTexture(1));

    unsigned int loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void RenderPass::UpdateVolumetricNoiseTexture(UID newTextureUID)
{
    if (newTextureUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newTextureUID) == nullptr)
    {
        newTextureUID = FALLBACK_TEXTURE_UID;
    }

    if (noiseTexture != nullptr && noiseTexture->GetUID() == newTextureUID) return;

    ResourceTexture* newTexture =
        dynamic_cast<ResourceTexture*>(App->GetResourcesModule()->RequestResource(newTextureUID));

    if (newTexture != nullptr)
    {

        App->GetResourcesModule()->ReleaseResource(noiseTexture);
        noiseTexture = newTexture;
    }
}

void RenderPass::RemoveVolumetricNoiseTexture()
{
    if (noiseTexture)
    {
        App->GetResourcesModule()->ReleaseResource(noiseTexture);
        useNoiseTexture = false;
        noiseTexture    = nullptr;
    }
}

void RenderPass::Resize(int width, int height) const
{
    if (fogResultTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, fogResultTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurrFBO[i]);
        glBindTexture(GL_TEXTURE_2D, blurrTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2, height / 2, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurrTextures[i], 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass::RenderShadowMapDebug() const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetDepthProgram();
    glUseProgram(program);

    unsigned int loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}