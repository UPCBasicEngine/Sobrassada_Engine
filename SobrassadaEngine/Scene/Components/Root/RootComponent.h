#pragma once
#include "Scene/Components/Component.h"

class RootComponent : public Component
{
public:
    RootComponent(const uint32_t uuid, const uint32_t ownerUUID, const char* name);

    bool AddComponent(const uint32_t componentUUID) override;
    bool RemoveComponent(const uint32_t componentUUID) override;
    void RenderEditorInspector() override;

    void Update() override;
};
