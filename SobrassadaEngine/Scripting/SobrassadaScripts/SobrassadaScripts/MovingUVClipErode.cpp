#include "pch.h"

#include "MovingUVClipErode.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/MeshComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "LightsConfig.h"
#include "Mesh.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "Math/float3.h"
#include "glew.h"

MovingUVClipErode::MovingUVClipErode(GameObject* parent) : Script(parent)
{
    fields.push_back({"Animation Speed", InspectorField::FieldType::Float, &animationSpeed, 0.f, 100.f});
    fields.push_back({"Moving UV Direction", InspectorField::FieldType::Vec2, &uvOffsetDirection, -1.f, 1.f});
    fields.push_back({"Double sided", InspectorField::FieldType::Bool, &isDoubleSided});
    fields.push_back({"Start UV Offset", InspectorField::FieldType::Vec2, &uvOffsetStart, -1.f, 1.f});
    fields.push_back({"Erosion Level", InspectorField::FieldType::Float, &erosionLevel, -1.0f, 1.0f});
    fields.push_back({"Edge Feather", InspectorField::FieldType::Float, &edgeFeather, 0.0f, 0.25f});
    fields.push_back({"Clip Min UV", InspectorField::FieldType::Vec2, &clipMin, -10.f, 10.f});
    fields.push_back({"Clip Max UV", InspectorField::FieldType::Vec2, &clipMax, -10.f, 10.f});
}


MovingUVClipErode::~MovingUVClipErode()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &materialBuffer);
}

bool MovingUVClipErode::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Custom/Vertex/MovingUV_Light_Vertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/MovingUV_ClipErode_Fragment.glsl"
    );

    meshComp = parent->GetComponent<MeshComponent*>();
    if (meshComp)
    {
        const ResourceMesh* rmesh = meshComp->GetResourceMesh();
        if (rmesh)
        {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
            glGenBuffers(1, &materialBuffer);

            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(
                GL_ARRAY_BUFFER, rmesh->GetLocalVertices().size() * sizeof(Vertex), rmesh->GetLocalVertices().data(),
                GL_STATIC_DRAW
            );

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, joint));

            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER, rmesh->GetIndices().size() * sizeof(unsigned int), rmesh->GetIndices().data(),
                GL_STATIC_DRAW
            );

            glBindVertexArray(0);

            indexCount = (unsigned int)rmesh->GetIndices().size();
        }

        const ResourceMaterial* rmat = meshComp->GetResourceMaterial();
        if (rmat)
        {
            isAlphaDiscard  = rmat->IsAlphaDiscard();
            MaterialGPU mat = rmat->GetMaterial();

            glBindBuffer(GL_UNIFORM_BUFFER, materialBuffer);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(mat), &mat, GL_STATIC_DRAW);
        }

        meshComp->SetEnabled(false);
        meshComp->SetUpdateShaderStorage(true);
    }

    uvOffset = uvOffsetStart;
    return true;
}

void MovingUVClipErode::Update(float dt)
{
    const float o  = dt * animationSpeed;
    uvOffset.x    += o * uvOffsetDirection.x;
    uvOffset.y    += o * uvOffsetDirection.y;
}

void MovingUVClipErode::Render(float dt, CameraComponent* cameraComp)
{
    if (!(shaderProgram && indexCount > 0 && meshComp)) return;

    float4x4 proj, view, model = meshComp->GetCombinedMatrix();
    if (cameraComp)
    {
        proj = cameraComp->GetProjectionMatrix();
        view = cameraComp->GetViewMatrix();
    }
    else
    {
        proj = AppEngine->GetCameraModule()->GetProjectionMatrix();
        view = AppEngine->GetCameraModule()->GetViewMatrix();
    }

    AppEngine->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

    glUseProgram(shaderProgram);

    // mateix layout que l’antic
    glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
    glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

    glUniform2fv(3, 1, &uvOffset[0]); // uUVOffset
    glUniform1i(4, 0);                // uMainTex sampler unit 0
    glUniform1i(5, isAlphaDiscard);   // uAlphaDiscard

    glUniform1i(9, meshComp->GetHasBones());
    glUniform1ui(10, meshComp->GetBoneIndexOffset());

    glBindBufferBase(GL_UNIFORM_BUFFER, 6, materialBuffer);

    GeometryBatch* batch = meshComp->GetBatch();
    if (batch) batch->BindBonesBuffer();

    float3 cameraPos = cameraComp ? cameraComp->GetCameraPosition() : AppEngine->GetCameraModule()->GetCameraPosition();
    glUniform3fv(6, 1, &cameraPos[0]);
    glUniform1f(7, erosionLevel); // uErode
    glUniform1f(8, edgeFeather);  // uEdgeFeather
    if (clipTo01)
    {
        float2 min01(0.f, 0.f), max01(1.f, 1.f);
        glUniform2fv(11, 1, &min01[0]); // uClipMin
        glUniform2fv(12, 1, &max01[0]); // uClipMax
    }
    else
    {
        glUniform2fv(11, 1, &clipMin[0]); // uClipMin
        glUniform2fv(12, 1, &clipMax[0]); // uClipMax
    }

    glBindVertexArray(vao);
    if (isDoubleSided) glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    if (isDoubleSided) glEnable(GL_CULL_FACE);
    if (batch) batch->BindBonesBuffer();
    glBindVertexArray(0);
}

void MovingUVClipErode::Reset()
{
    uvOffset = uvOffsetStart;
}
