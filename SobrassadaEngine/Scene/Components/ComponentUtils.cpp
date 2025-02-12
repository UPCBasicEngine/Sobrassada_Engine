#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Standalone/MeshComponent.h"

#include <cstdint>

Component * ComponentUtils::CreateEmptyComponent(ComponentType type, UID uid, UID uidParent, UID uidRoot, const Transform& parentGlobalTransform)
{
    switch (type)
    {
    case COMPONENT_NONE:
        return nullptr;
    case COMPONENT_ROOT:
        return new RootComponent(uid, uidParent, "Root component", parentGlobalTransform);
    case COMPONENT_MESH:
        return new MeshComponent(uid, uidParent, uidRoot, "Mesh renderer", parentGlobalTransform);
    }
    return nullptr;
}