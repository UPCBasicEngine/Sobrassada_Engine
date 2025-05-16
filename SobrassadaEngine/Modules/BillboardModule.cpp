#include "BillboardModule.h"

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

void BillboardModule::RequestTag(const std::string& tag, BillboardComponent* component)
{
}
