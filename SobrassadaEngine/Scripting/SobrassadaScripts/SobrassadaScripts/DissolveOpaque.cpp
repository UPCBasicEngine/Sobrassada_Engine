#include "pch.h"

#include "DissolveOpaque.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "Components/Standalone/MeshComponent.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "Mesh.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "ShaderModule.h"

#include "glew.h"

DissolveOpaque::DissolveOpaque(GameObject* parent) : Script(parent)
{
    fields.push_back({InspectorField::FieldType::Text, (void*)"Noise texture resource"});
    fields.push_back({"Noise texture", InspectorField::FieldType::Resource, &noiseTextureUID});

    fields.push_back({"Animation Duration", InspectorField::FieldType::Float, &dissolveDuration, 0.1f, 100.f});

    fields.push_back({"Glow Range", InspectorField::FieldType::Float, &glowRange, 0.f, 0.5f});
    fields.push_back({"Glow Fallof", InspectorField::FieldType::Float, &glowFallof, 0.f, 1.f});

    fields.push_back({"Glow Color", InspectorField::FieldType::Color, &glowColor});
}

DissolveOpaque::~DissolveOpaque()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

bool DissolveOpaque::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Custom/Vertex/DissolveOpaque.vert",
        "./EngineDefaults/Shader/Custom/Fragment/DissolveOpaque.frag"
    );

    noiseTexture = dynamic_cast<ResourceTexture*>(AppEngine->GetResourcesModule()->RequestResource(noiseTextureUID));

    meshComp     = parent->GetComponent<MeshComponent*>();

    if (meshComp)
    {
        const ResourceMesh* rmesh = meshComp->GetResourceMesh();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

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

        meshComp->SetEnabled(false);
        meshComp->SetUpdateShaderStorage(true);
    }

    if (meshComp && noiseTexture && shaderProgram) properlyInitialized = true;

    return properlyInitialized;
}

void DissolveOpaque::Update(float deltaTime)
{
    timer += deltaTime;
    if (timer >= dissolveDuration)
    {
        isFinished = true;
        meshComp->SetEnabled(true);
    }
}

void DissolveOpaque::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (!isFinished && properlyInitialized && shaderProgram)
    {
        if (meshComp->IsEnabled()) meshComp->SetEnabled(false);

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

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(0, 1, GL_TRUE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_TRUE, &viewMatrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &basicModelMatrix[0][0]);

        glUniform1f(3, timer / dissolveDuration);
        glUniform1i(4, meshComp->GetBaseIndex());
        glUniform1f(5, glowRange);
        glUniform1f(6, glowFallof);

        float4 color = float4(glowColor, 1.f);
        glUniform4fv(7, 1, &color[0]);

        glUniform1i(9, meshComp->GetHasBones());
        glUniform1ui(10, meshComp->GetBoneIndexOffset());

        GeometryBatch* batch = meshComp->GetBatch();
        if (batch)
        {
            batch->BindBonesBuffer();
            batch->BindMaterialsBuffer();
        }

        glBindVertexArray(vao);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, noiseTexture->GetTextureID());

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

        if (batch)
        {
            batch->UnbindBonesBuffer();
            batch->UnbindMaterialsBuffer();
        }
        glBindVertexArray(0);
    }
}

void DissolveOpaque::Reset()
{
    timer      = 0;
    isFinished = false;
    meshComp->SetEnabled(true);
}
