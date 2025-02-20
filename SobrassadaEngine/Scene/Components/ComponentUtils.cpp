﻿#include "ComponentUtils.h"

#include "Component.h"
#include "Root/RootComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Lights/PointLight.h"
#include "Standalone/Lights/SpotLight.h"
#include "Standalone/Lights/DirectionalLight.h"
#include "CubeMesh.h"

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
        return new PointLight(uid, uidParent, uidRoot, parentGlobalTransform);
    case COMPONENT_SPOT_LIGHT:
        return new SpotLight(uid, uidParent, uidRoot, parentGlobalTransform);
    case COMPONENT_DIRECTIONAL_LIGHT:
        return new DirectionalLight(uid, uidParent, uidRoot, parentGlobalTransform);
    case COMPONENT_CUBE_MESH:
        return new CubeMesh(uid, uidParent, uidRoot, parentGlobalTransform);
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
            return new PointLight(initialState);
        case COMPONENT_SPOT_LIGHT:
            return new SpotLight(initialState);
        case COMPONENT_DIRECTIONAL_LIGHT:
            return new DirectionalLight(initialState);
        default:
            return nullptr;
        }
    }
    return nullptr;
}