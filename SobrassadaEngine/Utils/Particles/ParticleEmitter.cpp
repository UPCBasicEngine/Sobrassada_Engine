#include "ParticleEmitter.h"

#include "BaseAddon.h"
#include "VelocityAddon.h"

#include "imgui.h"

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
    : uid(uid), name(name), owner(owner)
{
    addonTuple = std::make_tuple(ADDON_NULLPTR);
    ParticleUtils::CreateEmptyParticleAddon(ParticleAddonType::BASE, this);
}

ParticleEmitter::ParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner)
    : uid(uid), name(name), owner(owner)

{
    addonTuple = std::make_tuple(ADDON_NULLPTR);

    if (initialState.HasMember("UID")) uid = initialState["UID"].GetInt64();
    if (initialState.HasMember("Name")) name = initialState["Name"].GetString();
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
    ImGui::Text("Selected emitter: %s", name.c_str());
    if (ImGui::Button("Create Velocity")) AddAddon(ParticleAddonType::VELOCITY);

    std::apply([](auto&... pointer) { ((pointer ? pointer->RenderEditorInspector() : Nothing()), ...); }, addonTuple);
}

void ParticleEmitter::AddAddon(ParticleAddonType type)
{
    if (!createdAddons[(int)type - 1]) ParticleUtils::CreateEmptyParticleAddon(type, this);
}

void ParticleEmitter::RemoveAddon(ParticleAddonType type)
{
}
