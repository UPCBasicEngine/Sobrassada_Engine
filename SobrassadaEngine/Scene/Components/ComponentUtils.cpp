#include "ComponentUtils.h"

#include "CameraComponent.h"
#include "Component.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioListenerComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/BillboardComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/DecalComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/Lights/PointLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ButtonComponent.h"
#include "Standalone/UI/CanvasComponent.h"
#include "Standalone/UI/ImageComponent.h"
#include "Standalone/UI/Transform2DComponent.h"
#include "Standalone/UI/UILabelComponent.h"
#include "Standalone/UI/CanvasScalerComponent.h"


#include <cstdint>

void ComponentUtils::CreateEmptyComponent(const ComponentType type, const UID uid, GameObject* parent)
{
    Component* generatedComponent;
    auto& componentTuple = parent->GetComponentsTupleRef();
    switch (type)
    {
    case COMPONENT_NONE:
        return;
    case COMPONENT_MESH:
    {
        MeshComponent* mesh                      = new MeshComponent(uid, parent);
        std::get<MeshComponent*>(componentTuple) = mesh;
        generatedComponent                       = mesh;
        break;
    }
    case COMPONENT_POINT_LIGHT:
    {
        PointLightComponent* pointLight                = new PointLightComponent(uid, parent);
        std::get<PointLightComponent*>(componentTuple) = pointLight;
        generatedComponent                             = pointLight;
        break;
    }
    case COMPONENT_SPOT_LIGHT:
    {
        SpotLightComponent* spotLight                 = new SpotLightComponent(uid, parent);
        std::get<SpotLightComponent*>(componentTuple) = spotLight;
        generatedComponent                            = spotLight;
        break;
    }
    case COMPONENT_DIRECTIONAL_LIGHT:
    {
        DirectionalLightComponent* directionalLight          = new DirectionalLightComponent(uid, parent);
        std::get<DirectionalLightComponent*>(componentTuple) = directionalLight;
        generatedComponent                                   = directionalLight;
        break;
    }
    case COMPONENT_CHARACTER_CONTROLLER:
    {
        CharacterControllerComponent* characterController       = new CharacterControllerComponent(uid, parent);
        std::get<CharacterControllerComponent*>(componentTuple) = characterController;
        generatedComponent                                      = characterController;
        break;
    }
    case COMPONENT_TRANSFORM_2D:
    {
        Transform2DComponent* transform2d               = new Transform2DComponent(uid, parent);
        std::get<Transform2DComponent*>(componentTuple) = transform2d;
        generatedComponent                              = transform2d;
        break;
    }
    case COMPONENT_CANVAS:
    {
        CanvasComponent* canvas                    = new CanvasComponent(uid, parent);
        std::get<CanvasComponent*>(componentTuple) = canvas;
        generatedComponent                         = canvas;
        break;
    }
    case COMPONENT_LABEL:
    {
        UILabelComponent* uiLabel                   = new UILabelComponent(uid, parent);
        std::get<UILabelComponent*>(componentTuple) = uiLabel;
        generatedComponent                          = uiLabel;
        break;
    }
    case COMPONENT_CAMERA:
    {
        CameraComponent* camera                    = new CameraComponent(uid, parent);
        std::get<CameraComponent*>(componentTuple) = camera;
        generatedComponent                         = camera;
        break;
    }
    case COMPONENT_SCRIPT:
    {
        ScriptComponent* script                    = new ScriptComponent(uid, parent);
        std::get<ScriptComponent*>(componentTuple) = script;
        generatedComponent                         = script;
        break;
    }
    case COMPONENT_CUBE_COLLIDER:
    {
        CubeColliderComponent* cube                      = new CubeColliderComponent(uid, parent);
        std::get<CubeColliderComponent*>(componentTuple) = cube;
        generatedComponent                               = cube;
        break;
    }
    case COMPONENT_SPHERE_COLLIDER:
    {
        SphereColliderComponent* sphere                    = new SphereColliderComponent(uid, parent);
        std::get<SphereColliderComponent*>(componentTuple) = sphere;
        generatedComponent                                 = sphere;
        break;
    }
    case COMPONENT_CAPSULE_COLLIDER:
    {
        CapsuleColliderComponent* capsule                   = new CapsuleColliderComponent(uid, parent);
        std::get<CapsuleColliderComponent*>(componentTuple) = capsule;
        generatedComponent                                  = capsule;
        break;
    }
    case COMPONENT_ANIMATION:
    {
        AnimationComponent* animation                 = new AnimationComponent(uid, parent);
        std::get<AnimationComponent*>(componentTuple) = animation;
        generatedComponent                            = animation;
        break;
    }
    case COMPONENT_AIAGENT:
    {
        AIAgentComponent* aiAgent                   = new AIAgentComponent(uid, parent);
        std::get<AIAgentComponent*>(componentTuple) = aiAgent;
        generatedComponent                          = aiAgent;
        break;
    }
    case COMPONENT_IMAGE:
    {
        ImageComponent* image                     = new ImageComponent(uid, parent);
        std::get<ImageComponent*>(componentTuple) = image;
        generatedComponent                        = image;
        break;
    }
    case COMPONENT_BUTTON:
    {
        ButtonComponent* button                    = new ButtonComponent(uid, parent);
        std::get<ButtonComponent*>(componentTuple) = button;
        generatedComponent                         = button;
        break;
    }
    case COMPONENT_AUDIO_SOURCE:
    {
        AudioSourceComponent* audioSource               = new AudioSourceComponent(uid, parent);
        std::get<AudioSourceComponent*>(componentTuple) = audioSource;
        generatedComponent                              = audioSource;
        break;
    }
    case COMPONENT_AUDIO_LISTENER:
    {
        AudioListenerComponent* audioListener             = new AudioListenerComponent(uid, parent);
        std::get<AudioListenerComponent*>(componentTuple) = audioListener;
        generatedComponent                                = audioListener;
        break;
    }
    case COMPONENT_CANVAS_SCALER:
    {
        CanvasScalerComponent* canvasScaler               = new CanvasScalerComponent(uid, parent);
        std::get<CanvasScalerComponent*>(componentTuple) = canvasScaler;
        generatedComponent                                = canvasScaler;
        break;
    }
    case COMPONENT_BILLBOARD:
    {
        BillboardComponent* billboard                 = new BillboardComponent(uid, parent);
        std::get<BillboardComponent*>(componentTuple) = billboard;
        generatedComponent                            = billboard;
        break;
    }
    case COMPONENT_DECAL:
    {
        DecalComponent* decal                     = new DecalComponent(uid, parent);
        std::get<DecalComponent*>(componentTuple) = decal;
        generatedComponent                        = decal;
        break;
    }
    default:
        return;
    }
    parent->SetComponentCreated(type - 1);
    generatedComponent->Init();
}

void ComponentUtils::CreateExistingComponent(const rapidjson::Value& initialState, GameObject* parent)
{
    if (initialState.HasMember("Type"))
    {
        auto& componentTuple = parent->GetComponentsTupleRef();
        switch (initialState["Type"].GetInt())
        {
        case COMPONENT_NONE:
            return;
        case COMPONENT_MESH:
        {
            MeshComponent* mesh                      = new MeshComponent(initialState, parent);
            std::get<MeshComponent*>(componentTuple) = mesh;
            break;
        }
        case COMPONENT_POINT_LIGHT:
        {
            PointLightComponent* pointLight                = new PointLightComponent(initialState, parent);
            std::get<PointLightComponent*>(componentTuple) = pointLight;
            break;
        }
        case COMPONENT_SPOT_LIGHT:
        {
            SpotLightComponent* spotLight                 = new SpotLightComponent(initialState, parent);
            std::get<SpotLightComponent*>(componentTuple) = spotLight;
            break;
        }
        case COMPONENT_DIRECTIONAL_LIGHT:
        {
            DirectionalLightComponent* directionalLight          = new DirectionalLightComponent(initialState, parent);
            std::get<DirectionalLightComponent*>(componentTuple) = directionalLight;
            break;
        }
        case COMPONENT_CHARACTER_CONTROLLER:
        {
            CharacterControllerComponent* characterController = new CharacterControllerComponent(initialState, parent);
            std::get<CharacterControllerComponent*>(componentTuple) = characterController;
            break;
        }
        case COMPONENT_TRANSFORM_2D:
        {
            Transform2DComponent* transform2d               = new Transform2DComponent(initialState, parent);
            std::get<Transform2DComponent*>(componentTuple) = transform2d;
            break;
        }
        case COMPONENT_CANVAS:
        {
            CanvasComponent* canvas                    = new CanvasComponent(initialState, parent);
            std::get<CanvasComponent*>(componentTuple) = canvas;
            break;
        }
        case COMPONENT_LABEL:
        {
            UILabelComponent* uiLabel                   = new UILabelComponent(initialState, parent);
            std::get<UILabelComponent*>(componentTuple) = uiLabel;
            break;
        }
        case COMPONENT_CAMERA:
        {
            CameraComponent* camera                    = new CameraComponent(initialState, parent);
            std::get<CameraComponent*>(componentTuple) = camera;
            break;
        }
        case COMPONENT_SCRIPT:
        {
            ScriptComponent* script                    = new ScriptComponent(initialState, parent);
            std::get<ScriptComponent*>(componentTuple) = script;
            break;
        }
        case COMPONENT_CUBE_COLLIDER:
        {
            CubeColliderComponent* cube                      = new CubeColliderComponent(initialState, parent);
            std::get<CubeColliderComponent*>(componentTuple) = cube;
            break;
        }
        case COMPONENT_SPHERE_COLLIDER:
        {
            SphereColliderComponent* sphere                    = new SphereColliderComponent(initialState, parent);
            std::get<SphereColliderComponent*>(componentTuple) = sphere;
            break;
        }
        case COMPONENT_CAPSULE_COLLIDER:
        {
            CapsuleColliderComponent* capsule                   = new CapsuleColliderComponent(initialState, parent);
            std::get<CapsuleColliderComponent*>(componentTuple) = capsule;
            break;
        }
        case COMPONENT_ANIMATION:
        {
            AnimationComponent* animation                 = new AnimationComponent(initialState, parent);
            std::get<AnimationComponent*>(componentTuple) = animation;
            break;
        }
        case COMPONENT_AIAGENT:
        {
            AIAgentComponent* aiAgent                   = new AIAgentComponent(initialState, parent);
            std::get<AIAgentComponent*>(componentTuple) = aiAgent;
            break;
        }
        case COMPONENT_IMAGE:
        {
            ImageComponent* image                     = new ImageComponent(initialState, parent);
            std::get<ImageComponent*>(componentTuple) = image;
            break;
        }
        case COMPONENT_BUTTON:
        {
            ButtonComponent* button                    = new ButtonComponent(initialState, parent);
            std::get<ButtonComponent*>(componentTuple) = button;
            break;
        }
        case COMPONENT_AUDIO_SOURCE:
        {
            AudioSourceComponent* audioSource               = new AudioSourceComponent(initialState, parent);
            std::get<AudioSourceComponent*>(componentTuple) = audioSource;
            break;
        }
        case COMPONENT_AUDIO_LISTENER:
        {
            AudioListenerComponent* audioListener             = new AudioListenerComponent(initialState, parent);
            std::get<AudioListenerComponent*>(componentTuple) = audioListener;
            break;
        }
        case COMPONENT_BILLBOARD:
        {
            BillboardComponent* billboard                 = new BillboardComponent(initialState, parent);
            std::get<BillboardComponent*>(componentTuple) = billboard;
            break;
        }
        case COMPONENT_DECAL:
        {
            DecalComponent* decal                     = new DecalComponent(initialState, parent);
            std::get<DecalComponent*>(componentTuple) = decal;
            break;
        }
        default:
            return;
        }
        parent->SetComponentCreated(initialState["Type"].GetInt() - 1);
    }
}