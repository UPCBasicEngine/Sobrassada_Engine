#pragma once
#include "Globals.h"
#include "Transform.h"

#include <cstdint>
#include <map>
#include <string>

class Component;
class RootComponent;
class MeshComponent;

enum ComponentType
{
    // Empty type
    COMPONENT_NONE = 0,
    // Root types
    COMPONENT_ROOT = 1,
    // Standalone types
    COMPONENT_MESH = 2,
};

static const std::map<std::string, ComponentType> standaloneComponents = {
    {"Mesh", COMPONENT_MESH}
};

class ComponentUtils
{
public:
    static Component* CreateEmptyComponent(ComponentType type, UID uid, UID uidParent, UID uidRoot, const Transform& parentGlobalTransform);

    
};
