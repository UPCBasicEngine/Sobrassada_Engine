#pragma once
#include <cstdint>
#include <map>
#include <string>

class Component;
class RootComponent;
class MeshComponent;
class ModelComponent;

enum ComponentType
{
    // Empty type
    COMPONENT_NONE = 0,
    // Root types
    COMPONENT_ROOT = 1,
    // Standalone types
    COMPONENT_MODEL = 2,
    // DependentTypes
    COMPONENT_MESH = 3,
};

static const std::map<std::string, ComponentType> standaloneComponents = {
    {"Model", COMPONENT_MODEL}
};

class ComponentUtils
{
public:
    static Component* CreateEmptyComponent(ComponentType type, uint32_t uuid, uint32_t uuidParent, uint32_t uuidRoot);

    
};
