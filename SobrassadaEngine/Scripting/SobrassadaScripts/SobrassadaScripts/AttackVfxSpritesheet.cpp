#include "pch.h"

#include "AttackVfxSpritesheet.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/MeshComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "Mesh.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "glew.h"

AttackVfxSpritesheet::AttackVfxSpritesheet(GameObject* parent) : Script(parent)
{
    fields.push_back({"Cell width", InspectorField::FieldType::Float, &cellWidth, 0.f, 10000.f});
    fields.push_back({"Cell height", InspectorField::FieldType::Float, &cellHeight, 0.f, 1000.f});
    fields.push_back({"Update Rate", InspectorField::FieldType::Float, &updateRate, 0.0f, 1.0f});
    fields.push_back({"Row major", InspectorField::FieldType::Bool, &isRowMajor});
    fields.push_back({"Double sided", InspectorField::FieldType::Bool, &isDoubleSided});
    fields.push_back({"Is One Shot", InspectorField::FieldType::Bool, &isOneShot});
    fields.push_back({"Texture", InspectorField::FieldType::Resource, &otherImageUID});
}

AttackVfxSpritesheet::~AttackVfxSpritesheet()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glMakeTextureHandleNonResidentARB(otherImageBindlessUID);
    AppEngine->GetResourcesModule()->ReleaseResource(otherImage);
}

bool AttackVfxSpritesheet::Init()
{
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Custom/Vertex/AttackVfx_Vertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/AttackVfx_Fragment.glsl"
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

        meshComp->SetEnabled(false);
        meshComp->SetUpdateShaderStorage(true);

        if (otherImageUID == INVALID_UID || otherImage || otherImageBindlessUID != INVALID_UID) return true;

        otherImage = static_cast<ResourceTexture*>(AppEngine->GetResourcesModule()->RequestResource(otherImageUID));
        otherImageBindlessUID = glGetTextureHandleARB(otherImage->GetTextureID());
        glMakeTextureHandleResidentARB(otherImageBindlessUID);

        uvRange.x = 0.0f;
        uvRange.y = cellWidth / static_cast<float>(otherImage->GetTextureWidth());
        uvRange.z = 0.0f;
        uvRange.w = cellHeight / static_cast<float>(otherImage->GetTextureHeight());
    }
    return true;
}

void AttackVfxSpritesheet::Update(float deltaTime)
{
    timer += deltaTime;
    if (timer < updateRate) return;

    if (isRowMajor)
    {
        if (uvRange.y >= 1.0f)
        {
            uvRange.x  = 0.0f;
            uvRange.y  = cellWidth / static_cast<float>(otherImage->GetTextureWidth());

            uvRange.z += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            uvRange.w += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
        }
        else
        {
            uvRange.x += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
            uvRange.y += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
        }
    }
    else
    {
        if (uvRange.w >= 1.0f)
        {
            uvRange.z  = 0.0f;
            uvRange.w  = cellHeight / static_cast<float>(otherImage->GetTextureHeight());

            uvRange.x += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
            uvRange.y += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
        }
        else
        {
            uvRange.z += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            uvRange.w += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
        }
    }
    timer = 0.0f;

    if (isOneShot && uvRange.y >= 1.0f && uvRange.w >= 1.0f)
    {
        parent->SetEnabled(false);
    }
}

void AttackVfxSpritesheet::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (shaderProgram && indexCount > 0 && meshComp && meshComp->GetBatch())
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

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(0, 1, GL_TRUE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_TRUE, &viewMatrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &basicModelMatrix[0][0]);
        glUniform4fv(3, 1, uvRange.ptr());
        glUniform1i(9, meshComp->GetHasBones());
        glUniform1ui(10, meshComp->GetBoneIndexOffset());

        GeometryBatch* batch = meshComp->GetBatch();
        if (batch) batch->BindBonesBuffer();

        GLuint lower  = static_cast<GLuint>(otherImageBindlessUID & 0xFFFFFFFF);
        GLuint higher = static_cast<GLuint>(otherImageBindlessUID >> 32);
        glUniform2ui(4, lower, higher);

        glBindVertexArray(vao);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-10.0f, -10.0f);

        if (isDoubleSided) glDisable(GL_CULL_FACE);
        AppEngine->GetOpenGLModule()->DrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        if (isDoubleSided) glEnable(GL_CULL_FACE);

        glDisable(GL_POLYGON_OFFSET_FILL);
        if (batch) batch->UnbindBonesBuffer();
        glBindVertexArray(0);
    }
}

void AttackVfxSpritesheet::Reset()
{
    uvRange.x = 0.0f;
    uvRange.y = cellWidth / static_cast<float>(otherImage->GetTextureWidth());
    uvRange.z = 0.0f;
    uvRange.w = cellHeight / static_cast<float>(otherImage->GetTextureHeight());
}
