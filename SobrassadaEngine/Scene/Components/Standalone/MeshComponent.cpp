#include "MeshComponent.h"

#include "Application.h"
#include "BatchManager.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "LibraryModule.h"
#include "MeshImporter.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourceModel.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "Math/Quat.h"
#include "imgui.h"

MeshComponent::MeshComponent(const UID uid, GameObject* parent) : Component(uid, parent, "Mesh", COMPONENT_MESH)
{
}

MeshComponent::MeshComponent(const rapidjson::Value& initialState, GameObject* parent) : Component(initialState, parent)
{
    if (initialState.HasMember("Material"))
    {
        UID materialUID = initialState["Material"].GetUint64();
        if (materialUID != INVALID_UID) AddMaterial(materialUID);
    }
    if (initialState.HasMember("Mesh"))
    {
        AddMesh(initialState["Mesh"].GetUint64(), false);
    }
    if (initialState.HasMember("Bones"))
    {
        const rapidjson::Value& initBones = initialState["Bones"];
        for (unsigned int i = 0; i < initBones.Size(); ++i)
        {
            bonesUIDs.push_back(initBones[i].GetUint64());
        }
    }
    if (initialState.HasMember("ModelUID"))
    {
        modelUID = initialState["ModelUID"].GetUint64();

        // Get the skin index and the bind matrices
        if (initialState.HasMember("SkinIndex"))
        {
            ResourceModel* model = static_cast<ResourceModel*>(App->GetResourcesModule()->RequestResource(modelUID));
            skinIndex            = initialState["SkinIndex"].GetInt();
            if (model != nullptr) bindMatrices = model->GetModelData().GetSkin(skinIndex).inverseBindMatrices;
        }
    }

    if (!bonesUIDs.empty() && !bindMatrices.empty()) hasBones = true;
}

MeshComponent::~MeshComponent()
{
    App->GetResourcesModule()->ReleaseResource(currentMaterial);
    App->GetResourcesModule()->ReleaseResource(currentMesh);

    if (uniqueBatch) App->GetResourcesModule()->GetBatchManager()->RemoveBatch(batch);
}

void MeshComponent::Init()
{
    if (currentMesh != nullptr && currentMaterial != nullptr)
    {
        batch = App->GetResourcesModule()->GetBatchManager()->RequestBatch(this);
        batch->AddComponent(this);
    }
}

void MeshComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("Mesh", currentMesh != nullptr ? currentMesh->GetUID() : INVALID_UID, allocator);
    targetState.AddMember(
        "Material", currentMaterial != nullptr && !bUsesMeshDefaultMaterial ? currentMaterial->GetUID() : INVALID_UID,
        allocator
    );

    if (bones.size() > 0) // Store the skin of the mesh as the UID of each bone
    {
        rapidjson::Value valBones(rapidjson::kArrayType);
        for (const UID boneUID : bonesUIDs)
        {
            valBones.PushBack(boneUID, allocator);
        }

        targetState.AddMember("Bones", valBones, allocator);

        // Save the resource model UID used to load this mesh, so it can get the bind matrices when loading
        if (modelUID != INVALID_UID) targetState.AddMember("ModelUID", modelUID, allocator);
        if (skinIndex != -1) targetState.AddMember("SkinIndex", skinIndex, allocator);
    }
}

void MeshComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_MESH)
    {
        const MeshComponent* otherMesh = static_cast<const MeshComponent*>(other);
        enabled                        = otherMesh->enabled;
        wasEnabled                     = otherMesh->wasEnabled;

        UID otherMeshUID               = otherMesh->currentMesh ? otherMesh->currentMesh->GetUID() : INVALID_UID;
        UID otherMatUID = otherMesh->currentMaterial ? otherMesh->currentMaterial->GetUID() : INVALID_UID;

        AddMesh(otherMeshUID);
        AddMaterial(otherMatUID);

        modelUID     = otherMesh->modelUID;
        skinIndex    = otherMesh->skinIndex;
        bindMatrices = otherMesh->bindMatrices;
        hasBones     = otherMesh->hasBones;

        // Maybe it could be added to an existing batch, but probably is more costly than what is worth
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void MeshComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Mesh Component");

    ImGui::Text(currentMeshName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select mesh"))
    {
        ImGui::OpenPopup(CONSTANT_MESH_SELECT_DIALOG_ID);
    }

    if (ImGui::IsPopupOpen(CONSTANT_MESH_SELECT_DIALOG_ID))
    {
        AddMesh(App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
            CONSTANT_MESH_SELECT_DIALOG_ID, App->GetLibraryModule()->GetMeshMap(), INVALID_UID
        ));
    }

    ImGui::SeparatorText("Material");
    ImGui::Text(currentMaterialName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select material"))
    {
        ImGui::OpenPopup(CONSTANT_MATERIAL_SELECT_DIALOG_ID);
    }

    if (ImGui::IsPopupOpen(CONSTANT_MATERIAL_SELECT_DIALOG_ID))
    {

        const UID chosenMatUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
            CONSTANT_MATERIAL_SELECT_DIALOG_ID, App->GetLibraryModule()->GetMaterialMap(), INVALID_UID
        );

        if (chosenMatUID != INVALID_UID) AddMaterial(chosenMatUID);
    }

    if (currentMaterial != nullptr) currentMaterial->OnEditorUpdate();
}

void MeshComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (batch == nullptr && currentMesh != nullptr && currentMaterial != nullptr)
    {
        BatchEditorMode();
    }
}

void MeshComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void MeshComponent::RenderDebug(float deltaTime)
{
}

void MeshComponent::InitSkin()
{
    if (bones.size() > 0) return;
    for (UID uid : bonesUIDs)
    {
        bones.emplace_back(App->GetSceneModule()->GetScene()->GetGameObjectByUID(uid));
    }
}

void MeshComponent::AddMesh(UID resource, bool updateParent)
{
    if (resource == INVALID_UID) return;

    if (currentMesh != nullptr && currentMesh->GetUID() == resource) return;

    ResourceMesh* newMesh = dynamic_cast<ResourceMesh*>(App->GetResourcesModule()->RequestResource(resource));
    if (newMesh != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMesh);
        currentMeshName = newMesh->GetName();
        currentMesh     = newMesh;

        if (currentMaterial == nullptr)
        {
            const UID defaultMat = newMesh->GetDefaultMaterialUID();
            AddMaterial(defaultMat, true);
        }

        localComponentAABB = AABB(currentMesh->GetAABB());
        if (updateParent) parent->OnAABBUpdated();

        if (batch) BatchEditorMode();
    }
}

void MeshComponent::AddMaterial(UID resource, bool setDefaultMaterial)
{
    if (resource == INVALID_UID || App->GetResourcesModule()->RequestResource(resource) == nullptr)
    {
        resource = DEFAULT_MATERIAL_UID;
    }

    if (currentMaterial != nullptr && currentMaterial->GetUID() == resource) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(resource));
    if (newMaterial != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMaterial);
        currentMaterial          = newMaterial;
        currentMaterialName      = currentMaterial->GetName();
        bUsesMeshDefaultMaterial = setDefaultMaterial;

        if (batch) BatchEditorMode();
    }
}

void MeshComponent::BatchEditorMode()
{
    if (uniqueBatch) App->GetResourcesModule()->GetBatchManager()->RemoveBatch(batch);
    batch = App->GetResourcesModule()->GetBatchManager()->CreateNewBatch(this);
    batch->AddComponent(this);
    batch->LoadData();
    uniqueBatch = true;
}

void MeshComponent::OnTransformUpdated()
{
    combinedMatrix = parent->GetGlobalTransform();
    if (currentMesh != nullptr)
    {
        combinedMatrix = combinedMatrix * currentMesh->GetDefaultTransform();
    }
}