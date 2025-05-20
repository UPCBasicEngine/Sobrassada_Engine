#include "ParticleEmitter.h"

#include "BaseAddon.h"
#include "VelocityAddon.h"

// ---------- SECTION FOR TUPLE ITERATION ----------

// (Used in std apply for being able to use the ternary operator for nullptr's)
static void Nothing()
{
}

// SAVE ADDONS
template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type SaveAddonsTuple(
    const std::tuple<Tp...>& tuple, rapidjson::Value& addonsJSON, rapidjson::Document::AllocatorType& allocator
)
{
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type SaveAddonsTuple(
        const std::tuple<Tp...>& tuple, rapidjson::Value& addonsJSON, rapidjson::Document::AllocatorType& allocator
    )
{
    if (std::get<I>(tuple))
    {
        rapidjson::Value addonJSON(rapidjson::kObjectType);

        std::get<I>(tuple)->Save(addonJSON, allocator);

        addonsJSON.PushBack(addonJSON, allocator);
    }
    SaveAddonsTuple<I + 1, Tp...>(tuple, addonsJSON, allocator);
}

// ---------- END SECTION FOR TUPLE ITERATION ----------

ParticleEmitter::ParticleEmitter(UID uid, const std::string& name, ParticleSystemComponent* owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType::BASE, this);
}

ParticleEmitter::ParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType::BASE, this);
}

ParticleEmitter::~ParticleEmitter()
{
    std::apply([](auto&... tupleVar) { ((delete tupleVar, tupleVar = nullptr), ...); }, addonTuple);
}

void ParticleEmitter::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("UID", uid, allocator);
    targetState.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);

    rapidjson::Value addonsJSON(rapidjson::kArrayType);
    SaveAddonsTuple(addonTuple, addonsJSON, allocator);

    targetState.AddMember("Addons", addonsJSON, allocator);
}

void ParticleEmitter::Update(float deltaTime)
{
}

void ParticleEmitter::Spawn()
{
}

void ParticleEmitter::RenderEditor()
{
    std::apply([](auto&... pointer) { ((pointer ? pointer->RenderEditorInspector() : Nothing()), ...); }, addonTuple);
}

void ParticleEmitter::AddAddon(ParticleAddonType type)
{
    if (!createdAddons[(int)type - 1]) ParticleUtils::CreateEmptyParticleAddon(type, this);
}

void ParticleEmitter::RemoveAddon(ParticleAddonType type)
{
}
