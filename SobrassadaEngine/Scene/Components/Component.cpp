#include "Component.h"

#include "Application.h"
#include "ComponentUtils.h"
#include "SceneModule.h"

#include <Algorithm/Random/LCG.h>

Component::Component(const uint32_t uuid, const uint32_t uuidParent, const uint32_t uuidRoot, const char* name): uuid(uuid), uuidParent(uuidParent), uuidRoot(uuidRoot), name(name), enabled(true){}

void Component::Enable()
{
    enabled = true;
}

void Component::Disable()
{
    enabled = false;
}

bool Component::AddComponent(const uint32_t componentUUID)
{
    children.push_back(componentUUID);
    return true;
}

bool Component::RemoveComponent(const uint32_t componentUUID)
{
    if (const auto it = std::find(children.begin(), children.end(), componentUUID); it != children.end())
    {
        children.erase(it);
        return true;
    }
    return false;
}

void Component::RenderEditorInspector()
{
}

void Component::RenderEditorComponentTree(const uint32_t selectedComponentUUID)
{
}
