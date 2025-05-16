#include "Billboard.h"

#include "Application.h"
#include "ResourceMaterial.h"
#include "ResourcesModule.h"
#include "Standalone/BillboardComponent.h"

#include "glew.h"

Billboard::Billboard(float width, float height) : width(width), height(height)
{
    CreateVertexBufferObject();
}

Billboard::~Billboard()
{
    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->ClearBillboardData();
    }

    if (material) App->GetResourcesModule()->ReleaseResource(material);
    glDeleteBuffers(1, &vbo);
}

void Billboard::UpdateWidth(float newWidth)
{
    width = newWidth;
    CreateVertexBufferObject();

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetWidth(width);
    }
}

void Billboard::UpdateHeight(float newHeight)
{
    height = newHeight;
    CreateVertexBufferObject();

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetHeight(height);
    }
}

void Billboard::UpdateMaterial(UID newMaterialUID)
{
    if (newMaterialUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newMaterialUID) == nullptr)
    {
        newMaterialUID = DEFAULT_MATERIAL_UID;
    }

    if (material != nullptr && material->GetUID() == newMaterialUID) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(newMaterialUID));

    if (newMaterial != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(material);
        material     = newMaterial;

        for (auto billboardComponent : instanceComponents)
        {
            billboardComponent->SetMaterial(material);
        }
    }
}

void Billboard::CreateVertexBufferObject()
{
    // vertices -> texture coords

    float vertexData[] = {
        -width / 2.f, height / 2.f,  0.f, //
        -width / 2.f, -height / 2.f, 0.f, //
        width / 2.f,  -height / 2.f, 0.f, //

        -width / 2.f, height / 2.f,  0.f, //
        width / 2.f,  -height / 2.f, 0.f, //
        width / 2.f,  height / 2.f,  0.f, //

        0.f,          1.f, //
        0.f,          0.f, //
        1.f,          0.f, //

        0.f,          1.f, //
        1.f,          0.f, //
        1.f,          1.f, //
    };

    if (vbo == 0) glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
}

void Billboard::AddComponent(BillboardComponent* newBillboard)
{
    auto iterator = instanceComponents.insert(instanceComponents.end(), newBillboard);

    newBillboard->SetWidth(width);
    newBillboard->SetHeight(height);
    newBillboard->SetMaterial(material);
    newBillboard->SetIterator(iterator);
}

void Billboard::RemoveComponent(std::list<BillboardComponent*>::iterator billboardIterator)
{
    instanceComponents.erase(billboardIterator);
}
