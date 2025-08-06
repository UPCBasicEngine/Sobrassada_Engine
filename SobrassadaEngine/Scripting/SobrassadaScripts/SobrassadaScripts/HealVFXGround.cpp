#include "pch.h"

#include "HealVFXGround.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/MeshComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "LightsConfig.h"
#include "Mesh.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "InputModule.h" // TODO: DELETE

#include "Math/float3.h"
#include "glew.h"

HealVFXGround::HealVFXGround(GameObject* parent, const std::string& ver, const std::string& frag) : Script(parent)
{
    vertex   = ver;
    fragment = frag;
    fields.push_back({"Animation Speed", InspectorField::FieldType::Int, &animationFPS, 0, 100});
}

HealVFXGround::~HealVFXGround()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &materialBuffer);
}

bool HealVFXGround::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(vertex.c_str(), fragment.c_str());

    meshComp      = parent->GetComponent<MeshComponent*>();

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

            xd              = rmat->GetDiffuseColorID();

            glBindBuffer(GL_UNIFORM_BUFFER, materialBuffer);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(mat), &mat, GL_STATIC_DRAW);
        }

        meshComp->SetEnabled(false);
    }
    return true;
}

void HealVFXGround::Update(float deltaTime)
{
    // Calculate the frame since beginning and pass it to shader (Maybe just a timer and adjust its speed to match the
    // animation)

    // TODO: DELETE
    if (AppEngine->GetInputModule()->GetKeyboard()[SDL_SCANCODE_F4] == KeyState::KEY_DOWN)
    {
        Reset();
    }

    frameTimer += deltaTime * animationFPS;
}

void HealVFXGround::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (shaderProgram && indexCount > 0 && meshComp)
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

        float2 uvOffset(0.0f, 0.0f);
        glUniform2fv(3, 1, &uvOffset[0]);

        glUniform1i(4, 0);
        glUniform1i(5, true);
        glBindBufferBase(GL_UNIFORM_BUFFER, 6, materialBuffer);

        float3 cameraPos = float3::zero;
        if (cameraComp == nullptr) cameraPos = AppEngine->GetCameraModule()->GetCameraPosition();
        else cameraPos = cameraComp->GetCameraPosition();

        glUniform3fv(6, 1, &cameraPos[0]);
        glUniform1f(7, frameTimer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, xd);

        glBindVertexArray(vao);

        AppEngine->GetOpenGLModule()->DrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }
}

void HealVFXGround::Reset()
{
    frameTimer = 0.0f;
}
