﻿#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Lights/PointLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"

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
    case COMPONENT_POINT_LIGHT:
        return new PointLightComponent(uid, uidParent, uidRoot, parentGlobalTransform);
    case COMPONENT_SPOT_LIGHT:
        return new SpotLightComponent(uid, uidParent, uidRoot, parentGlobalTransform);
    case COMPONENT_DIRECTIONAL_LIGHT:
        return new DirectionalLightComponent(uid, uidParent, uidRoot, parentGlobalTransform);
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
        case COMPONENT_POINT_LIGHT:
            return new PointLightComponent(initialState);
        case COMPONENT_SPOT_LIGHT:
            return new SpotLightComponent(initialState);
        case COMPONENT_DIRECTIONAL_LIGHT:
            return new DirectionalLightComponent(initialState);
        default:
            return nullptr;
        }
    }
    return nullptr;
}