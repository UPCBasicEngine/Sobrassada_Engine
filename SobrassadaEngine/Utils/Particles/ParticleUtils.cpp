#include "ParticleUtils.h"

#include "BaseAddon.h"
#include "ParticleEmitter.h"
#include "VelocityAddon.h"
#include "SpritesheetAddon.h"

void ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType type, ParticleEmitter* emitter)
{
    auto& tuple = emitter->GetAddonsTuple();
    switch (type)
    {
    case ParticleAddonType::NONE:
        return;
    case ParticleAddonType::BASE:
    {
        BaseAddon* addon            = new BaseAddon(emitter);
        std::get<BaseAddon*>(tuple) = addon;
        break;
    }
    case ParticleAddonType::VELOCITY:
    {
        VelocityAddon* addon            = new VelocityAddon(emitter);
        std::get<VelocityAddon*>(tuple) = addon;
        break;
    }
    case ParticleAddonType::SPRITESHEET:
    {
        SpritesheetAddon* addon         = new SpritesheetAddon(emitter);
        std::get<SpritesheetAddon*>(tuple) = addon;
        break;
    }
    default:
        return;
    }
    emitter->SetAddonCreated((int)type - 1);
}

void ParticleUtils::CreateExistingComponent(const rapidjson::Value& initialState, ParticleEmitter* emitter)
{
    if (!initialState.HasMember("AddonType")) return;
    ParticleAddonType type = ParticleAddonType(initialState["AddonType"].GetInt());

    auto& tuple            = emitter->GetAddonsTuple();
    switch (type)
    {
    case ParticleAddonType::NONE:
        return;
    case ParticleAddonType::BASE:
    {
        BaseAddon* addon            = new BaseAddon(initialState, emitter);
        std::get<BaseAddon*>(tuple) = addon;
        break;
    }
    case ParticleAddonType::VELOCITY:
    {
        VelocityAddon* addon            = new VelocityAddon(initialState, emitter);
        std::get<VelocityAddon*>(tuple) = addon;
        break;
    }
    case ParticleAddonType::SPRITESHEET:
    {
        SpritesheetAddon* addon            = new SpritesheetAddon(initialState, emitter);
        std::get<SpritesheetAddon*>(tuple) = addon;
        break;
    }
    default:
        break;
    }
    emitter->SetAddonCreated((int)type - 1);
}
