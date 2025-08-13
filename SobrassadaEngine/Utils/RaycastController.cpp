#include "RaycastController.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "FileSystem/Mesh.h"
#include "GameObject.h"
#include "ResourceMesh.h"
#include "SceneModule.h"
#include "Standalone/BillboardComponent.h"
#include "Standalone/MeshComponent.h"

#include "Geometry/Triangle.h"
#include "Math/float4x4.h"
#include <algorithm>
#include <vector>

namespace RaycastController
{
    GameObject* GetRayIntersectionObject(const LineSegment& ray, const std::vector<GameObject*>& queriedGameObjects)
    {
        GameObject* selectedGameObject = nullptr;

        std::vector<GameObject*> aabbIntersectedObjects;

        // GETTING GAMEOBJECTS THAT INTERSECT WITH THE RAY
        float closeDistance = 0;
        float farDistance   = 0;
        for (const auto& gameObject : queriedGameObjects)
        {
            if (gameObject->IsGloballyEnabled() &&
                ray.Intersects(gameObject->GetGlobalAABB(), closeDistance, farDistance))
            {
                aabbIntersectedObjects.push_back(gameObject);
            }
        }

        float closestDistance = std::numeric_limits<float>::infinity();

        // FOREACH GAMEOBJECT INTERSECTING CHECKING AGAINST THE RAY
        for (const auto& gameObject : aabbIntersectedObjects)
        {
            LineSegment localRay(ray.a, ray.b);

            // CHECK FOR GAME OBJECTS WITH MESHES
            const MeshComponent* meshComponent = gameObject->GetComponent<MeshComponent*>();
            if (meshComponent != nullptr)
            {
                const ResourceMesh* resourceMesh = meshComponent->GetResourceMesh();
                if (!resourceMesh) continue;  

                float4x4 globalTransform         = meshComponent->GetCombinedMatrix();
                globalTransform.Inverse();
                localRay.Transform(globalTransform);

                const std::vector<unsigned int>& indices = resourceMesh->GetIndices();
                const std::vector<Vertex>& vertices      = resourceMesh->GetLocalVertices();
                if (indices.size() < 3 || vertices.empty())
                    continue;

                for (size_t i = 2; i < indices.size(); i += 3) 
                {
                    unsigned int indexA = indices[i - 2];
                    unsigned int indexB = indices[i - 1];
                    unsigned int indexC = indices[i];

                    if (indexA >= vertices.size() || indexB >= vertices.size() || indexC >= vertices.size())
                        continue; 

                    const float3& firstVertex  = vertices[indexA].position;
                    const float3& secondVertex = vertices[indexB].position;
                    const float3& thirdVertex  = vertices[indexC].position;

                    Triangle currentTriangle(firstVertex, secondVertex, thirdVertex);

                    float distance = std::numeric_limits<float>::infinity();
                    float3 hitPoint;
                    if (localRay.Intersects(currentTriangle, &distance, &hitPoint) && distance < closestDistance)
                    {
                        closestDistance    = distance;
                        selectedGameObject = gameObject;
                    }
                }
            }

            // CHECK IN CASE GAME OBJECT IS A BILLBOARD
            const BillboardComponent* billboardComponent = gameObject->GetComponent<BillboardComponent*>();
            if (billboardComponent != nullptr)
            {

                bool playMode               = App->GetSceneModule()->GetInPlayMode();

                const Frustum& editorCamera = App->GetCameraModule()->GetCamera();
                CameraComponent* gameCamera = App->GetSceneModule()->GetScene()->GetMainCamera();

                float3 cameraPosition;
                float3 rightVector;
                float3 upVector;

                if (playMode)
                {
                    if (!gameCamera) continue;
                    cameraPosition = gameCamera->GetCameraPosition();
                    rightVector    = gameCamera->GetCameraRight();
                    upVector       = gameCamera->GetCameraUp();
                }
                else
                {
                    cameraPosition = editorCamera.pos;
                    rightVector    = editorCamera.WorldRight();
                    upVector       = editorCamera.up;
                }

                float3 position    = billboardComponent->GetParent()->GetGlobalTransform().TranslatePart();

                float3 frontVector = cameraPosition - position;
                frontVector.Normalize();

                float3x3 rotationMatrix = float3x3(
                    rightVector, billboardComponent->GetLockPitch() ? float3(0, 1.f, 0) : upVector, frontVector
                );

                const float4x4& originalTransform = billboardComponent->GetParent()->GetLocalTransform();
                float4x4 newLocalTransform = float4x4::FromTRS(position, rotationMatrix, originalTransform.GetScale());

                newLocalTransform.Inverse();
                localRay.Transform(newLocalTransform);

                float width  = billboardComponent->GetWidth();
                float height = billboardComponent->GetHeight();

                Triangle billboardTriangles[2];

                billboardTriangles[0] = Triangle(
                    float3(-width / 2.f, height / 2.f, 0.f), float3(-width / 2.f, -height / 2.f, 0.f),
                    float3(width / 2.f, -height / 2.f, 0.f)
                );
                billboardTriangles[1] = Triangle(
                    float3(-width / 2.f, height / 2.f, 0.f), float3(width / 2.f, -height / 2.f, 0.f),
                    float3(width / 2.f, height / 2.f, 0.f)
                );

                // Billboards are just 2 triangles
                for (int i = 0; i < 2; ++i)
                {
                    float distance = std::numeric_limits<float>::infinity();
                    float3 hitPoint;

                    if (localRay.Intersects(billboardTriangles[i], &distance, &hitPoint))
                    {
                        if (distance < closestDistance)
                        {
                            closestDistance    = distance;
                            selectedGameObject = gameObject;
                        }
                    }
                }
            }
        }

        if (selectedGameObject && selectedGameObject->HasSelectParent())
        {
            SceneModule* sceneModule     = App->GetSceneModule();

            UID rootGameObject           = sceneModule->GetScene()->GetGameObjectRootUID();
            GameObject* parentGameobject = sceneModule->GetScene()->GetGameObjectByUID(selectedGameObject->GetParent());

            if (parentGameobject)
            {
                while (parentGameobject && parentGameobject->GetUID() != rootGameObject &&
                       parentGameobject->HasSelectParent() &&
                       parentGameobject->GetUID() != sceneModule->GetScene()->GetMultiselectUID())
                {
                    selectedGameObject = parentGameobject;
                    parentGameobject   = sceneModule->GetScene()->GetGameObjectByUID(selectedGameObject->GetParent());
                }

                if (parentGameobject && !parentGameobject->HasSelectParent()) selectedGameObject = parentGameobject;
            }
        }

        if (selectedGameObject && !App->GetSceneModule()->GetScene()->IsMultiselecting())
            selectedGameObject->UpdateOpenNodeHierarchy(true);

        return selectedGameObject;
    }
} // namespace RaycastController
