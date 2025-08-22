#include "ShaderScriptModule.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "Framebuffer.h"
#include "Gbuffer.h"
#include "OpenGLModule.h"
#include "GameObject.h"
#include "Scene/Components/ShaderScriptComponent.h"

#include "glew.h"

#include <algorithm>

ShaderScriptModule::ShaderScriptModule()
{
}

ShaderScriptModule::~ShaderScriptModule()
{
}

bool ShaderScriptModule::Init()
{
    return true;
}

bool ShaderScriptModule::ShutDown()
{
    return true;
}

void ShaderScriptModule::AddShaderScript(
    ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType shaderType
)
{
    switch (shaderType)
    {
    case ShaderScriptType::GEOMERTY_PASS:
        geometryPassComponents.push_back({component, scriptIndex});
        break;
    case ShaderScriptType::TRANSPARENT_PASS:
        transparentComponents.push_back({component, scriptIndex});
        break;
    case ShaderScriptType::POST_LIGHTING_PASS:
        postLightingComponents.push_back({component, scriptIndex});
        break;
    case ShaderScriptType::POST_EFFECTS_PASS:
        postEffectsComponents.push_back({component, scriptIndex});
        break;
    default:
        break;
    }
}

void ShaderScriptModule::ShaderScriptTypeChange(
    ShaderScriptComponent* component, unsigned int scriptIndex, ShaderScriptType previous, ShaderScriptType newType
)
{
    std::vector<std::pair<ShaderScriptComponent*, unsigned int>>* originalVector = &geometryPassComponents;

    switch (previous)
    {
    case ShaderScriptType::GEOMERTY_PASS:
        originalVector = &geometryPassComponents;
        break;
    case ShaderScriptType::TRANSPARENT_PASS:
        originalVector = &transparentComponents;
        break;
    case ShaderScriptType::POST_LIGHTING_PASS:
        originalVector = &postLightingComponents;
        break;
    case ShaderScriptType::POST_EFFECTS_PASS:
        originalVector = &postEffectsComponents;
        break;
    default:
        return;
        break;
    }

    int originalVectorIndex = -1;

    for (int i = 0; i < originalVector->size(); ++i)
    {
        if (component == (*originalVector)[i].first && scriptIndex == (*originalVector)[i].second)
        {
            originalVectorIndex = i;
            break;
        }
    }

    if (originalVectorIndex > -1)
    {
        originalVector->erase(originalVector->begin() + originalVectorIndex);

        std::vector<std::pair<ShaderScriptComponent*, unsigned int>>* destinationVector = &geometryPassComponents;
        switch (newType)
        {
        case ShaderScriptType::GEOMERTY_PASS:
            destinationVector = &geometryPassComponents;
            break;
        case ShaderScriptType::TRANSPARENT_PASS:
            destinationVector = &transparentComponents;
            break;
        case ShaderScriptType::POST_LIGHTING_PASS:
            destinationVector = &postLightingComponents;
            break;
        case ShaderScriptType::POST_EFFECTS_PASS:
            destinationVector = &postEffectsComponents;
            break;
        default:
            return;
            break;
        }

        destinationVector->push_back({component, scriptIndex});
    }
}

void ShaderScriptModule::ComponentDeleted(ShaderScriptComponent* component)
{
    std::vector<std::vector<std::pair<ShaderScriptComponent*, unsigned int>>::iterator> iteratorsToRemove;

    for (auto iterator = geometryPassComponents.begin(); iterator != geometryPassComponents.end(); ++iterator)
    {
        if (iterator->first == component)
        {
            iteratorsToRemove.push_back(iterator);
        }
    }

    for (auto& iterator : iteratorsToRemove)
    {
        geometryPassComponents.erase(iterator);
    }

    iteratorsToRemove.clear();

    for (auto iterator = transparentComponents.begin(); iterator != transparentComponents.end(); ++iterator)
    {
        if (iterator->first == component)
        {
            iteratorsToRemove.push_back(iterator);
        }
    }

    for (auto& iterator : iteratorsToRemove)
    {
        transparentComponents.erase(iterator);
    }

    iteratorsToRemove.clear();

    for (auto iterator = postLightingComponents.begin(); iterator != postLightingComponents.end(); ++iterator)
    {
        if (iterator->first == component)
        {
            iteratorsToRemove.push_back(iterator);
        }
    }

    for (auto& iterator : iteratorsToRemove)
    {
        postLightingComponents.erase(iterator);
    }

    iteratorsToRemove.clear();

    for (auto iterator = postEffectsComponents.begin(); iterator != postEffectsComponents.end(); ++iterator)
    {
        if (iterator->first == component)
        {
            iteratorsToRemove.push_back(iterator);
        }
    }

    for (auto& iterator : iteratorsToRemove)
    {
        postEffectsComponents.erase(iterator);
    }

    iteratorsToRemove.clear();
}

void ShaderScriptModule::ComponentDeletedScript(ShaderScriptComponent* component)
{
    ComponentDeleted(component);

    auto& componentScriptsTypes = component->GetScriptRenderTypes();

    for (int i = 0; i < componentScriptsTypes.size(); ++i)
    {
        switch (componentScriptsTypes[i])
        {
        case ShaderScriptType::GEOMERTY_PASS:
            geometryPassComponents.push_back({component, i});
            break;
        case ShaderScriptType::TRANSPARENT_PASS:
            transparentComponents.push_back({component, i});
            break;
        case ShaderScriptType::POST_LIGHTING_PASS:
            postLightingComponents.push_back({component, i});
            break;
        case ShaderScriptType::POST_EFFECTS_PASS:
            postEffectsComponents.push_back({component, i});
            break;
        default:
            break;
        }
    }
}

void ShaderScriptModule::RenderGeometryPassShaders(float deltaTime, CameraComponent* camera)
{
    GBuffer* gbuffer = App->GetOpenGLModule()->GetGBuffer();

    if (!gbuffer) return;

    gbuffer->Bind();

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    for (auto& shaderPair : geometryPassComponents)
    {
        shaderPair.first->RenderScript(deltaTime, camera, shaderPair.second);
    }

    glEnable(GL_BLEND);

    gbuffer->Unbind();
}

void ShaderScriptModule::RenderTransparentPassShaders(float deltaTime, CameraComponent* camera)
{
    // SORT MESHES TO CAMERA DISTABCE
    std::sort(
        transparentComponents.begin(), transparentComponents.end(),
        [camera](
            const std::pair<ShaderScriptComponent*, unsigned int>& a,
            const std::pair<ShaderScriptComponent*, unsigned int>& b
        )
        {
            if (camera != nullptr)
            {
                float distanceA =
                    (a.first->GetParent()->GetGlobalTransform().TranslatePart() - camera->GetCameraPosition())
                        .LengthSq();
                float distanceB =
                    (b.first->GetParent()->GetGlobalTransform().TranslatePart() - camera->GetCameraPosition())
                        .LengthSq();

                return distanceA > distanceB;
            }
            else
            {
                float distanceA =
                    (a.first->GetParent()->GetGlobalTransform().TranslatePart() - App->GetCameraModule()->GetCameraPosition())
                        .LengthSq();
                float distanceB =
                    (b.first->GetParent()->GetGlobalTransform().TranslatePart() - App->GetCameraModule()->GetCameraPosition())
                        .LengthSq();

                return distanceA > distanceB;
            }
        }
    );

    Framebuffer* framebuffer = App->GetOpenGLModule()->GetFramebuffer();

#ifndef GAME
    framebuffer->Bind();
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (auto& shaderPair : transparentComponents)
    {
        shaderPair.first->RenderScript(deltaTime, camera, shaderPair.second);
    }

    glDisable(GL_BLEND);
}

void ShaderScriptModule::RenderPostLightingPassShaders(float deltaTime, CameraComponent* camera)
{
    for (auto& shaderPair : postLightingComponents)
    {
        shaderPair.first->RenderScript(deltaTime, camera, shaderPair.second);
    }
}

void ShaderScriptModule::RenderPostEffectsPassShaders(float deltaTime, CameraComponent* camera)
{
    for (auto& shaderPair : postEffectsComponents)
    {
        shaderPair.first->RenderScript(deltaTime, camera, shaderPair.second);
    }
}
