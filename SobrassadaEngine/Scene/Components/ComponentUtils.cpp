#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Dependent/MeshComponent.h"
#include "Standalone/ModelComponent.h"

#include <cstdint>

Component * ComponentUtils::CreateEmptyComponent(ComponentType type, uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot)
{
    switch (type)
    {
    case COMPONENT_NONE:
        return nullptr;
    case COMPONENT_ROOT:
        return new RootComponent(uuid, uuidParent, "root");
    case COMPONENT_MODEL:
        return new ModelComponent(uuid, uuidParent, uuidRoot, "model");
    case COMPONENT_MESH:
        return new MeshComponent(uuid, uuidParent, uuidRoot, "mesh");
    }
    return nullptr;
}