#include "BillboardModule.h"

#include "Billboard.h"
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
        Billboard* newBillboard = new Billboard(component->GetWidth(), component->GetWidth());
        newBillboard->UpdateMaterial(component->GetMaterialUID());
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
    for (auto& billboard : billboardMap)
    {
        billboard.second.second->Render();
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

void BillboardModule::UpdateTagPositions(const HashString& tag)
{
    auto billboardIterator = billboardMap.find(tag);

    if (billboardIterator != billboardMap.end()) billboardIterator->second.second->SetReloadPositions();
}

bool BillboardModule::FindTag(const HashString& tag, int& outPosition)
{
    if (billboardTags.empty())
    {
        outPosition = 0;
        return false;
    }

    int left  = 0;
    int right = billboardTags.size() - 1;
    while (left <= right)
    {
        outPosition = (right + left) / 2;
        if (billboardTags[outPosition] == tag) return true;
        if (billboardTags[outPosition] < tag) left = outPosition + 1;
        else if (billboardTags[outPosition] > tag) right = outPosition - 1;
    }

    return false;
}
