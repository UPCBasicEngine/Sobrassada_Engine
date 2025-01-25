#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Dependent/MeshComponent.h"
#include "Standalone/ModelComponent.h"

#include <cstdint>

Component * ComponentUtils::CreateEmptyComponent(ComponentType type, uint32_t uuid, uint32_t ownerUUID)
{
    switch (type)
    {
    case COMPONENT_NONE:
        return nullptr;
    case COMPONENT_ROOT:
        return new RootComponent(uuid, ownerUUID, "root");
    case COMPONENT_MODEL:
        return new ModelComponent(uuid, ownerUUID, "model");
    case COMPONENT_MESH:
        return new MeshComponent(uuid, ownerUUID, "mesh");
    }
    return nullptr;
}