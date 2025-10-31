#include "pch.h"

#include "MirageVFX.h"

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

MirageVFX::MirageVFX(GameObject* parent, const std::string& ver, const std::string& frag) : Script(parent)
{
    vertex   = ver;
    fragment = frag;
    fields.push_back({"Animation Speed", InspectorField::FieldType::Float, &animationFPS, 0.0f, 100.0f});
    fields.push_back({"Additive", InspectorField::FieldType::Bool, &isAdditive});
    fields.push_back({"Color 1", InspectorField::FieldType::Vec3, &color1, 0.0f, 1.0f});
    fields.push_back({"Color 2", InspectorField::FieldType::Vec3, &color2, 0.0f, 1.0f});
    fields.push_back({"Color 3", InspectorField::FieldType::Vec3, &color3, 0.0f, 1.0f});
    fields.push_back({"Color 4", InspectorField::FieldType::Vec3, &color4, 0.0f, 1.0f});
}

MirageVFX::~MirageVFX()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

bool MirageVFX::Init()
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

        meshComp->SetEnabled(false);
    }
    return true;
}

void MirageVFX::Update(float deltaTime)
{
    // TODO: DELETE
    if (AppEngine->GetInputModule()->GetKeyboard()[SDL_SCANCODE_F4] == KeyState::KEY_DOWN)
    {
        Reset();
    }

    frameTimer += deltaTime * animationFPS;
}

void MirageVFX::Render(float deltaTime, CameraComponent* cameraComp)
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

        glUniform1f(4, frameTimer);
        glUniform3f(5, color1.x, color1.y, color1.z);
        glUniform3f(6, color2.x, color2.y, color2.z);
        glUniform3f(7, color3.x, color3.y, color3.z);
        glUniform3f(8, color4.x, color4.y, color4.z);

        glBindVertexArray(vao);

        if (isAdditive) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        else glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_CULL_FACE);
        AppEngine->GetOpenGLModule()->DrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(0);
    }
}

void MirageVFX::Reset()
{
    frameTimer = 0.0f;
}

void MirageVFX ::SetAllColors(const float3& newColor)
{
    color1 = color2 = color3 = color4 = newColor;
}