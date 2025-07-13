#include "pch.h"
#include "BossMirage.h"

BossMirage::BossMirage(GameObject* parent) : Script(parent)
{
    fields.push_back(
        {"Gather Sequence",
         [this](Script* self)
         {
             //find every mirage script and save to a sequence
         }}
    );
}

bool BossMirage::Init()
{
    return false;
}

void BossMirage::Update(float deltaTime)
{

}

