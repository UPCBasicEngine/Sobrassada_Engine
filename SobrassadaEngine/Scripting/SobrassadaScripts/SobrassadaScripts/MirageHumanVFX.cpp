#include "pch.h"

#include "MirageHumanVFX.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/MeshComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "InputModule.h" // TODO: DELETE
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

MirageHumanVFX::MirageHumanVFX(GameObject* parent, const std::string& ver, const std::string& frag) : Script(parent)
{
    vertex   = ver;
    fragment = frag;
    fields.push_back({"Additive", InspectorField::FieldType::Bool, &isAdditive});
    fields.push_back({"Color Tint", InspectorField::FieldType::Vec3, &colorTint});
}

MirageHumanVFX::~MirageHumanVFX()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

bool MirageHumanVFX::Init()
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
    }
    return true;
}

void MirageHumanVFX::Update(float deltaTime)
{
}

void MirageHumanVFX::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (shaderProgram && indexCount > 0 && meshComp)
    {
        float4x4 projectionMatrix, viewMatrix, basicModelMatrix;

        basicModelMatrix = meshComp->GetCombinedMatrix();
        meshComp->SetEnabled(false);

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

        glUniform3fv(3, 1, &colorTint[0]);

        glUniform1i(9, meshComp->GetHasBones());
        glUniform1ui(10, meshComp->GetBoneIndexOffset());

        GeometryBatch* batch = meshComp->GetBatch();
        if (batch) batch->BindBonesBuffer();

        glBindVertexArray(vao);

        if (isAdditive) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        else glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, -2.0f);

        AppEngine->GetOpenGLModule()->DrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

        glDisable(GL_POLYGON_OFFSET_FILL);
        if (batch) batch->UnbindBonesBuffer();
        glBindVertexArray(0);
    }
}
