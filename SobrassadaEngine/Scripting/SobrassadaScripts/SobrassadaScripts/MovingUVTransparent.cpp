#include "pch.h"

#include "MovingUVTransparent.h"

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

MovingUVTransparent::MovingUVTransparent(GameObject* parent) : Script(parent)
{
    fields.push_back({"Animation Speed", InspectorField::FieldType::Float, &animationSpeed, 0.f, 100.f});
    fields.push_back({"Moving UV Direction", InspectorField::FieldType::Vec2, &uvOffsetDirection, -1.f, 1.f});
    fields.push_back({"Double sided", InspectorField::FieldType::Bool, &isDoubleSided});
    fields.push_back({"Start UV Offset", InspectorField::FieldType::Vec2, &uvOffsetStart, -1.f, 1.f});
}

MovingUVTransparent::~MovingUVTransparent()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &materialBuffer);
}

bool MovingUVTransparent::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Custom/Vertex/MovingUV_Light_Vertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/MovingUV_Transparent_Fragment.glsl"
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

void MovingUVTransparent::Update(float deltaTime)
{
    float newOffset  = deltaTime * animationSpeed;
    uvOffset.x      += newOffset * uvOffsetDirection.x;
    uvOffset.y      += newOffset * uvOffsetDirection.y;
}

void MovingUVTransparent::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (!(shaderProgram && indexCount > 0 && meshComp)) return;
    if (meshComp->GetHasBones() && !meshComp->GetBatch()) return;
    {
        float4x4 projectionMatrix, viewMatrix, basicModelMatrix;

        basicModelMatrix = meshComp->GetCombinedMatrix();

        if (cameraComp)
        {
            projectionMatrix = cameraComp->GetProjectionMatrix();
            viewMatrix       = cameraComp->GetViewMatrix();
        }
        else
        {
            projectionMatrix = AppEngine->GetCameraModule()->GetProjectionMatrix();
            viewMatrix       = AppEngine->GetCameraModule()->GetViewMatrix();
        }

        AppEngine->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(0, 1, GL_TRUE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_TRUE, &viewMatrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &basicModelMatrix[0][0]);
        glUniform2fv(3, 1, &uvOffset[0]);
        glUniform1i(9, meshComp->GetHasBones());
        glUniform1ui(10, meshComp->GetBoneIndexOffset());

        glUniform1i(4, 0);
        glUniform1i(5, isAlphaDiscard);

        glBindBufferBase(GL_UNIFORM_BUFFER, 6, materialBuffer);

        GeometryBatch* batch = meshComp->GetBatch();
        if (batch) batch->BindBonesBuffer();

        float3 cameraPos = float3::zero;
        if (cameraComp == nullptr) cameraPos = AppEngine->GetCameraModule()->GetCameraPosition();
        else cameraPos = cameraComp->GetCameraPosition();

        glUniform3fv(6, 1, &cameraPos[0]);

        glBindVertexArray(vao);

        if (isDoubleSided) glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        if (isDoubleSided) glEnable(GL_CULL_FACE);

        if (batch) batch->BindBonesBuffer();

        glBindVertexArray(0);
    }
}

void MovingUVTransparent::Reset()
{
    uvOffset = uvOffsetStart;
}
