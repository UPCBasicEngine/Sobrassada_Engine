#include "pch.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/MeshComponent.h"
#include "GameObject.h"
#include "Mesh.h"
#include "RenderTestScript.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "glew.h"

RenderTestScript::~RenderTestScript()
{
    if (shaderProgram)
    {
        AppEngine->GetShaderModule()->DeleteProgram(shaderProgram);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

bool RenderTestScript::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->CreateShaderProgram(
        "./EngineDefaults/Shader/Vertex/TestVertex.glsl", "./EngineDefaults/Shader/Fragment/TestFragment.glsl"
    );
    MeshComponent* meshComp = parent->GetComponent<MeshComponent*>();

    if (meshComp)
    {
        const ResourceMesh* rmesh = meshComp->GetResourceMesh();

        if (rmesh)
        {
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

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER, rmesh->GetIndices().size() * sizeof(unsigned int), rmesh->GetIndices().data(),
                GL_STATIC_DRAW
            );

            glBindVertexArray(0);

            indexCount = rmesh->GetIndices().size();
        }
    }

    return true;
}

void RenderTestScript::Update(float deltaTime)
{
}

void RenderTestScript::Render(float deltaTime, CameraComponent* cameraComp)
{

    float4x4 projectionMatrix, viewMatrix, basicModelMatrix;

    basicModelMatrix = parent->GetGlobalTransform();

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

    glBindVertexArray(vao);

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}
