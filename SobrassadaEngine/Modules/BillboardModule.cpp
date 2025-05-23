#include "BillboardModule.h"

#include "Application.h"
#include "Billboard.h"
#include "CameraComponent.h"
#include "SceneModule.h"
#include "Standalone/BillboardComponent.h"

BillboardModule::BillboardModule()
{
}

BillboardModule::~BillboardModule()
{
}

bool BillboardModule::Init()
{
    return true;
}

bool BillboardModule::ShutDown()
{
    return true;
}

update_status BillboardModule::PostUpdate(float deltaTime)
{

    for (auto& billboard : billboardMap)
    {
        billboard.second.second->CheckReloadPositions();
    }

    return UPDATE_CONTINUE;
}

update_status BillboardModule::Update(float deltaTime)
{
    bool playMode               = App->GetSceneModule()->GetInPlayMode();
    const Frustum& editorCamera = App->GetCameraModule()->GetCamera();
    const CameraComponent* gameCamera =
        App->GetSceneModule()->IsSceneLoaded() ? App->GetSceneModule()->GetScene()->GetMainCamera() : nullptr;

    float3 cameraPosition =
        playMode ? gameCamera ? gameCamera->GetCameraPosition() : editorCamera.pos : editorCamera.pos;

    for (auto& billboard : billboardMap)
    {
        billboard.second.second->UpdatePositionsVbo(cameraPosition);
    }

    return UPDATE_CONTINUE;
}

void BillboardModule::CreateTag(const char* newTag)
{
    HashString tag = HashString(newTag);
    if (tag == emptyString) return;

    if (billboardMap.find(tag) == billboardMap.end())
    {
        billboardTags.push_back(tag);
    }
}

void BillboardModule::DeleteTag(const HashString tag)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end())
    {
        delete billboardIterator->second.second;
        billboardMap.erase(billboardIterator);
    }

    for (auto tagIterator = billboardTags.begin(); tagIterator != billboardTags.end(); ++tagIterator)
    {
        if (*tagIterator == tag)
        {
            billboardTags.erase(tagIterator);
            return;
        }
    }
}

void BillboardModule::RequestTag(const HashString& tag, BillboardComponent* component)
{
    int position            = -1;
    bool found              = FindTag(tag, position);
    HashString componentTag = component->GetBillboardTag();

    // If tag is not found it means it must be loaded for the first time
    if (!found) billboardTags.insert(billboardTags.begin() + position, tag);

    if (tag != componentTag && emptyString != componentTag) RemoveComponentFromTag(componentTag, component);

    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator == billboardMap.end())
    {
        Billboard* newBillboard = new Billboard(component->GetWidth(), component->GetHeight());
        if (component->IsUsingTexture()) newBillboard->UpdateTexture(component->GetTextureUID());
        else newBillboard->UpdateMaterial(component->GetMaterialUID());
        newBillboard->UpdateLockPitch(component->GetLockPitch());
        newBillboard->UpdateUVCoords(
            component->GetXmin(), component->GetYmin(), component->GetSelectionWidth(), component->GetSelectionHeight()
        );
        newBillboard->AddComponent(component);

        billboardMap.insert({tag, std::pair<unsigned int, Billboard*>(1, newBillboard)});
    }
    else
    {
        billboardIterator->second.first++;
        billboardIterator->second.second->AddComponent(component);
    }
}

void BillboardModule::RemoveComponentFromTag(const HashString& tag, BillboardComponent* component)
{
    auto billboardIterator = billboardMap.find(tag);
    if (billboardIterator != billboardMap.end())
    {
        billboardIterator->second.first--;
        billboardIterator->second.second->RemoveComponent(component->GetBillboardIterator());

        if (billboardIterator->second.first == 0) DeleteTag(tag);
    }
}

void BillboardModule::RenderBillboards()
{

    bool playMode                     = App->GetSceneModule()->GetInPlayMode();
    const Frustum& editorCamera       = App->GetCameraModule()->GetCamera();
    const CameraComponent* gameCamera = App->GetSceneModule()->GetScene()->GetMainCamera();

    float4x4 VP;
    float3 rightVector;
    float3 upVector;

    if (playMode && gameCamera)
    {
        VP          = gameCamera->GetProjectionMatrix() * gameCamera->GetViewMatrix();
        rightVector = gameCamera->GetCameraRight();
        upVector    = gameCamera->GetCameraUp();
    }
    else
    {
        VP          = editorCamera.ProjectionMatrix() * editorCamera.ViewMatrix();
        rightVector = editorCamera.WorldRight();
        upVector    = editorCamera.up;
    }

    for (auto& billboard : billboardMap)
    {
        billboard.second.second->Render(VP, rightVector, upVector);
    }
}

void BillboardModule::UpdateTagWidth(const HashString& tag, float width)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateWidth(width);
}

void BillboardModule::UpdateTagHeight(const HashString& tag, float height)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateHeight(height);
}

void BillboardModule::UpdateTagMaterial(const HashString& tag, UID material)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateMaterial(material);
}

void BillboardModule::UpdateTagTexture(const HashString& tag, UID texture)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateTexture(texture);
}

void BillboardModule::UpdateTagLockPitch(const HashString& tag, bool lockAxis)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateLockPitch(lockAxis);
}

void BillboardModule::UpdateTagUseTexture(const HashString& tag, bool useTexture)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->UpdateUseTexture(useTexture);
}

void BillboardModule::UpdateTagPositions(const HashString& tag)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->SetReloadPositions();
}

void BillboardModule::UpdateTagUVCoords(
    const HashString& tag, float xmin, float ymin, float selectionWidth, float selectionHeight
)
{
    auto billboardIterator = billboardMap.find(tag);
    if (billboardIterator != billboardMap.end())
        billboardIterator->second.second->UpdateUVCoords(xmin, ymin, selectionWidth, selectionHeight);
}

bool BillboardModule::FindTag(const HashString& tag, int& outPosition)
{
    if (billboardTags.empty())
    {
        outPosition = 0;
        return false;
    }

    int left  = 0;
    int right = (int)billboardTags.size() - 1;
    while (left <= right)
    {
        outPosition = (right + left) / 2;
        if (billboardTags[outPosition] == tag) return true;
        if (billboardTags[outPosition] < tag) left = outPosition + 1;
        else if (billboardTags[outPosition] > tag) right = outPosition - 1;
    }

    return false;
}
