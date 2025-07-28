#pragma once

#include "Script.h"
#include "HashString.h"

class PlayerLocationScript : public Script
{
  public:
    PlayerLocationScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override {}

    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void OnCollisionExit(GameObject* otherObject, ColliderLayer layer) override;

  private:
    std::string locationTagString;
    HashString locationTag;

};
