#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Standalone/MeshComponent.h"

#include <cstdint>

Component * ComponentUtils::CreateEmptyComponent(ComponentType type, uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot, const Transform& parentGlobalTransform)
{
    switch (type)
    {
    case COMPONENT_NONE:
        return nullptr;
    case COMPONENT_ROOT:
        return new RootComponent(uuid, uuidParent, "Root component", parentGlobalTransform);
    case COMPONENT_MESH:
        return new MeshComponent(uuid, uuidParent, uuidRoot, "Mesh renderer", parentGlobalTransform);
    }
    return nullptr;
}