#include "BillboardModule.h"

#include "Billboard.h"

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

void BillboardModule::CreateTag(const char* newTag)
{
    HashString tag = HashString(newTag);

    if (billboardMap.find(tag) == billboardMap.end())
    {
        billboardTags.push_back(tag);
    }
}

void BillboardModule::DeleteTag(const HashString& tag)
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
}
