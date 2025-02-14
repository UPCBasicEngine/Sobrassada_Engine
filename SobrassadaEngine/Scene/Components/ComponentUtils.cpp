#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Standalone/MeshComponent.h"

#include <cstdint>

Component *ComponentUtils::CreateEmptyComponent(
    ComponentType type, UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform
)
{
    switch (type)
    {
    case COMPONENT_NONE:
        return nullptr;
    case COMPONENT_ROOT:
        return new RootComponent(uid, uidParent, parentGlobalTransform);
    case COMPONENT_MESH:
        return new MeshComponent(uid, uidParent, uidRoot, parentGlobalTransform);
    }
    return nullptr;
}

Component *ComponentUtils::CreateExistingComponent(const rapidjson::Value &initialState)
{
    if (initialState.HasMember("Type"))
    {
        switch (initialState["Type"].GetInt())
        {
        case COMPONENT_NONE:
            return nullptr;
        case COMPONENT_ROOT:
            return new RootComponent(initialState);
        case COMPONENT_MESH:
            return new MeshComponent(initialState);
        default:
            return nullptr;
        }
    }
    return nullptr;
}