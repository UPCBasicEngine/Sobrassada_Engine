#include "ParticleAddon.h"

ParticleAddon::ParticleAddon(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("Enabled")) isEnabled = initialState["Enabled"].GetBool();
    if (initialState.HasMember("AddonType")) addonType = ParticleAddonType(initialState["AddonType"].GetInt());
}

void ParticleAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("Enabled", isEnabled, allocator);
    targetState.AddMember("AddonType", (int)addonType, allocator);
}
