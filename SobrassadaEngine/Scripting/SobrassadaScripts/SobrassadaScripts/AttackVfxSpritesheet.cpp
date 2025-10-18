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
#include "ShaderScriptComponent.h"

#include "glew.h"

AttackVfxSpritesheet::AttackVfxSpritesheet(GameObject* parent) : Script(parent)
{
    fields.push_back({"Cell width", InspectorField::FieldType::Float, &cellWidth, 0.f, 10000.f});
    fields.push_back({"Cell height", InspectorField::FieldType::Float, &cellHeight, 0.f, 10000.f});
    fields.push_back({"Update Rate", InspectorField::FieldType::Float, &updateRate, 0.0f, 1.0f});
    fields.push_back({"Row major", InspectorField::FieldType::Bool, &isRowMajor});
    fields.push_back({"Double sided", InspectorField::FieldType::Bool, &isDoubleSided});
    fields.push_back({"Is One Shot", InspectorField::FieldType::Bool, &isOneShot});
    fields.push_back({"Only Once", InspectorField::FieldType::Bool, &onlyOnce});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Row column parameters"});
    fields.push_back({"Use row columns", InspectorField::FieldType::Bool, &useRowCol});
    fields.push_back({"Rows", InspectorField::FieldType::Int, &rows, 1, 100});
    fields.push_back({"Colums", InspectorField::FieldType::Int, &cols, 1, 100});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Animation duration parameters"});
    fields.push_back({"Use animation duration", InspectorField::FieldType::Bool, &useAnimDuration});
    fields.push_back({"Animation Duration", InspectorField::FieldType::Float, &animationDuration, 0.1f, 100.f});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Texture resource"});
    fields.push_back({"Texture", InspectorField::FieldType::Resource, &otherImageUID});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Spritesheet Variations"});
    fields.push_back({"Number of variations to use", InspectorField::FieldType::Int, &variationsToUse});
    fields.push_back({"Variation 1", InspectorField::FieldType::Resource, &variationsUID1});
    fields.push_back({"Variation 2", InspectorField::FieldType::Resource, &variationsUID2});
    fields.push_back({"Variation 3", InspectorField::FieldType::Resource, &variationsUID3});
    fields.push_back({"Variation 4", InspectorField::FieldType::Resource, &variationsUID4});
}

AttackVfxSpritesheet::~AttackVfxSpritesheet()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glMakeTextureHandleNonResidentARB(otherImageBindlessUID);
    AppEngine->GetResourcesModule()->ReleaseResource(otherImage);

    for (int i = 0; i < variationsToUse; ++i)
    {
        glMakeTextureHandleNonResidentARB(variationsBindlessUID[i]);
        AppEngine->GetResourcesModule()->ReleaseResource(variations[i]);
    }
}

bool AttackVfxSpritesheet::Init()
{
    if (isInitialized) return true;
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
        if (otherImage)
        {
            otherImageBindlessUID = glGetTextureHandleARB(otherImage->GetTextureID());
            glMakeTextureHandleResidentARB(otherImageBindlessUID);

            if (!useRowCol)
            {
                uvRange.x = 0.0f;
                uvRange.y = cellWidth / static_cast<float>(otherImage->GetTextureWidth());
                uvRange.z = 0.0f;
                uvRange.w = cellHeight / static_cast<float>(otherImage->GetTextureHeight());

                step.x    = cellWidth / static_cast<float>(otherImage->GetTextureWidth());
                step.y    = cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            }
            else
            {
                uvRange.x = 0.0f;
                uvRange.y = 1.0f / float(cols);
                uvRange.z = 0.0f;
                uvRange.w = 1.0f / float(rows);

                step.x    = 1.0f / float(cols);
                step.y    = 1.0f / float(rows);
            }

            ResetUVs(otherImage);
        }

        UID variationsUID[4] = {variationsUID1, variationsUID2, variationsUID3, variationsUID4};

        for (int i = 0; i < variationsToUse; ++i)
        {
            variations[i] =
                static_cast<ResourceTexture*>(AppEngine->GetResourcesModule()->RequestResource(variationsUID[i]));

            if (variations[i])
            {
                variationsBindlessUID[i] = glGetTextureHandleARB(variations[i]->GetTextureID());
                glMakeTextureHandleResidentARB(variationsBindlessUID[i]);
            }
        }

        // Start using the default image
        currentImageUID = otherImageBindlessUID;
    }

    if (animationDuration <= 0.f) animationDuration = 0.1f;
    isInitialized = true;
    return true;
}

void AttackVfxSpritesheet::Update(float deltaTime)
{
    // This sometimes doesn't get called, so logic is in the render
}

void AttackVfxSpritesheet::Render(float deltaTime, CameraComponent* cameraComp)
{
    UpdateSprite(deltaTime);

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

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(0, 1, GL_TRUE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(1, 1, GL_TRUE, &viewMatrix[0][0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &basicModelMatrix[0][0]);
        glUniform4fv(3, 1, uvRange.ptr());
        glUniform1i(9, meshComp->GetHasBones());
        glUniform1ui(10, meshComp->GetBoneIndexOffset());

        GeometryBatch* batch = meshComp->GetBatch();
        if (batch) batch->BindBonesBuffer();

        GLuint lower  = static_cast<GLuint>(currentImageUID & 0xFFFFFFFF);
        GLuint higher = static_cast<GLuint>(currentImageUID >> 32);
        glUniform2ui(4, lower, higher);

        glBindVertexArray(vao);

        if (isAdditive) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, -2.0f);

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
    if (!useRowCol)
    {
        uvRange.x = 0.0f;
        uvRange.y = step.x;
        uvRange.z = 0.0f;
        uvRange.w = step.y;
    }
    else
    {
        uvRange.x = 0.0f;
        uvRange.y = step.x;
        uvRange.z = 0.0f;
        uvRange.w = step.y;
    }

    finished = false;
    if (variationsToUse > 0)
    {
        int idx = rand() % (variationsToUse + 1);
        GLOG("IDX: %d", idx);
        if (idx == variationsToUse)
        {
            currentImageUID = otherImageBindlessUID;
            ResetUVs(otherImage);
        }
        else
        {
            currentImageUID = variationsBindlessUID[idx];
            ResetUVs(variations[idx]);
        }
    }
    else
    {
        ResetUVs(otherImage);
    }
}

const bool AttackVfxSpritesheet::AlmostFinished(int specificRow, int specificCol) const
{
    int actualRow = static_cast<int>(uvRange.z * otherImage->GetTextureHeight() / cellHeight);
    int actualCol = static_cast<int>(uvRange.x * otherImage->GetTextureWidth() / cellWidth);

    if (useRowCol)
    {
        actualRow = int(uvRange.z * float(rows));
        actualCol = int(uvRange.x * float(cols));
    }

    if (isRowMajor)
    {
        if (actualRow > specificRow || actualRow == specificRow && actualCol >= specificCol) return true;

        return false;
    }
    else
    {
        if (actualCol > specificCol || actualCol == specificCol && actualRow >= specificRow) return true;

        return false;
    }
}

void AttackVfxSpritesheet::UpdateSprite(float deltaTime)
{
    if (!otherImage) return;

    timer += deltaTime;
    if (!useAnimDuration)
    {
        if (timer < updateRate) return;
    }
    else
    {
        if (timer < (animationDuration / float(rows * cols))) return;
    }
    if (timer < updateRate) return;
    else if (finished) return;

    if (isRowMajor)
    {
        if (uvRange.y >= 1.0f)
        {
            uvRange.x  = 0.0f;
            // uvRange.y  = cellWidth / static_cast<float>(otherImage->GetTextureWidth());
            uvRange.y  = step.x;

            /*uvRange.z += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            uvRange.w += cellHeight / static_cast<float>(otherImage->GetTextureHeight());*/

            uvRange.z += step.y;
            uvRange.w += step.y;
        }
        else
        {
            /*uvRange.x += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
            uvRange.y += cellWidth / static_cast<float>(otherImage->GetTextureWidth());*/

            uvRange.x += step.x;
            uvRange.y += step.x;
        }
    }
    else
    {
        if (uvRange.w >= 1.0f)
        {
            uvRange.z  = 0.0f;
            // uvRange.w  = cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            uvRange.w  = step.y;

            // uvRange.x += cellWidth / static_cast<float>(otherImage->GetTextureWidth());
            // uvRange.y += cellWidth / static_cast<float>(otherImage->GetTextureWidth());

            uvRange.x += step.x;
            uvRange.y += step.x;
        }
        else
        {
            // uvRange.z += cellHeight / static_cast<float>(otherImage->GetTextureHeight());
            // uvRange.w += cellHeight / static_cast<float>(otherImage->GetTextureHeight());

            uvRange.z += step.y;
            uvRange.w += step.y;
        }
    }

    timer = 0.0f;

    if (isOneShot && uvRange.y >= 1.0f && uvRange.w >= 1.0f)
    {
        parent->GetComponent<ShaderScriptComponent*>()->SetEnabled(false);
    }

    if (onlyOnce && uvRange.y >= 1.0f && uvRange.w >= 1.0f)
    {
        finished = true;
    }
}

void AttackVfxSpritesheet::ResetUVs(ResourceTexture* tex)
{
    if (!tex) return;

    if (useRowCol)
    {
        uvRange.y = 1.0f / float(cols);
        uvRange.w = 1.0f / float(rows);
    }
    else
    {
        uvRange.y = cellWidth / static_cast<float>(tex->GetTextureWidth());
        uvRange.w = cellHeight / static_cast<float>(tex->GetTextureHeight());
    }

    uvRange.x = 0.0f;
    uvRange.z = 0.0f;

    finished  = false;
}