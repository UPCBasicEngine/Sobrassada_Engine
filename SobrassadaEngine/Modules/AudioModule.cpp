#include "AudioModule.h"

#include "Application.h"
#include "Components/Standalone/Audio/AudioListenerComponent.h"
#include "Components/Standalone/Audio/AudioSourceComponent.h"
#include "FileSystem.h"
#include "Globals.h"
#include "ProjectModule.h"


#include "AK/IBytes.h"
#include "AK/MusicEngine/Common/AkMusicEngine.h"
#include "AK/SoundEngine/Common/AkMemoryMgr.h"       // Memory Manager interface
#include "AK/SoundEngine/Common/AkMemoryMgrModule.h" // Default memory manager
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "AK/SoundEngine/Common/AkStreamMgrModule.h"
#include "AK/SoundEngine/Common/IAkStreamMgr.h"    // Streaming Manager
#include "AK/SpatialAudio/Common/AkSpatialAudio.h" // Spatial Audio
#include "rapidjson/document.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

AudioModule::AudioModule()
{
    g_envMAP[0] = 0; // hardcode environment 0 to id=0
    for (int i = 1; i < 255; i++)
    {
        g_envMAP[i] = -1;
    }
}

AudioModule::~AudioModule()
{
}

bool AudioModule::Init()
{
    AkMemSettings memSettings;
    AK::MemoryMgr::GetDefaultSettings(memSettings);

    AkStreamMgrSettings stmSettings;
    AK::StreamMgr::GetDefaultSettings(stmSettings);

    AkDeviceSettings deviceSettings;
    AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

    AkInitSettings l_InitSettings;
    AkPlatformInitSettings l_platInitSetings;
    AK::SoundEngine::GetDefaultInitSettings(l_InitSettings);
    AK::SoundEngine::GetDefaultPlatformInitSettings(l_platInitSetings);

    AkMusicSettings musicInit;
    AK::MusicEngine::GetDefaultInitSettings(musicInit);

    AkSpatialAudioInitSettings settings;

    // Create and initialise an instance of our memory manager.
    if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
    {
        GLOG("Could not create the memory manager.");
        return false;
    }
    // Create and initialise an instance of the default stream manager.
    if (!AK::StreamMgr::Create(stmSettings))
    {
        GLOG("Could not create the Stream Manager");
        return false;
    }
    // Create an IO device.
    if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
    {
        GLOG("Cannot create streaming I/O device");
        return false;
    }
    // Initialize sound engine.
    if (AK::SoundEngine::Init(&l_InitSettings, &l_platInitSetings) != AK_Success)
    {
        GLOG("Cannot initialize sound engine");
        return false;
    }
    // Initialize music engine.
    if (AK::MusicEngine::Init(&musicInit) != AK_Success)
    {
        GLOG("Cannot initialize music engine");
        return false;
    }
    if (AK::SpatialAudio::Init(settings) != AK_Success)
    {
        GLOG("Could not initialize the Spatial Audio.");
        return false;
    }

    return true;
}

void AudioModule::InitAudio()
{
    loadedAudio                      = true;
    const std::string soundbanksPath = App->GetProjectModule()->GetLoadedProjectPath() + WINDOWS_BANKS_PATH;

    const int size                   = MultiByteToWideChar(CP_UTF8, 0, soundbanksPath.c_str(), -1, nullptr, 0);
    std::wstring wSoundbanksPath(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, soundbanksPath.c_str(), -1, &wSoundbanksPath[0], size);

#ifdef AK_WIN
    g_lowLevelIO.SetBasePath(wSoundbanksPath.c_str());
#else
    g_lowLevelIO.SetBasePath(AKTEXT("soundbanks/Mac/"));
#endif
    AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)"));
#ifndef AK_OPTIMIZED
    // Initialize communications (not in release build!)
    AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings(commSettings);
    if (AK::Comm::Init(commSettings) != AK_Success)
    {
        GLOG("Could not initialize communication.");
        return;
    }
#endif

    AkBankID bankID;
    AKRESULT retValue;
    retValue = AK::SoundEngine::LoadBank(BANKNAME_INIT, bankID);
    if (retValue != AK_Success)
    {
        GLOG("Cannot initialize Init.bnk soundbank");
        return;
    }
    retValue = AK::SoundEngine::LoadBank(BANKNAME_MAIN, bankID);
    if (retValue != AK_Success)
    {
        GLOG("Cannot initialize main.bnk soundbank");
        return;
    }

    //  initialize volume parameters to sensible default values
    musicvol(255);
    soundvol(255);
    voicevol(255);

    ParseEvents();

    for (const auto& source : sources)
    {
        source->UpdateEventsNames();
    }
}

update_status AudioModule::Update(float deltaTime)
{
    // Do this here because if trying to load soundbanks when no project loaded, no soundbanks are found and sounds
    // don't work
    if (!App->GetProjectModule()->IsProjectLoaded())
    {
        if (loadedAudio) UnloadBanks();
        return UPDATE_CONTINUE;
    }
    if (!loadedAudio) InitAudio();

    if (listener)
    {
        // Set the listener position
        AkSoundPosition listenerPos;
        const float3& position   = listener->GetGlobalTransform().TranslatePart();
        const float3x3& rotation = listener->GetGlobalTransform().RotatePart();

        listenerPos.SetPosition({position.x, position.y, position.z});
        listenerPos.SetOrientation(
            {rotation.Col(2).x, rotation.Col(2).y, rotation.Col(2).z},
            {rotation.Col(1).x, rotation.Col(1).y, rotation.Col(1).z}
        );

        AK::SoundEngine::SetPosition(listener->GetParentUID(), listenerPos);
    }

    // Set the sources position
    for (const auto source : sources)
    {
        // Set the source position
        AkSoundPosition sourcePos;
        const float3& position   = source->GetGlobalTransform().TranslatePart();
        const float3x3& rotation = source->GetGlobalTransform().RotatePart();

        sourcePos.SetPosition({position.x, position.y, position.z});
        sourcePos.SetOrientation(
            {rotation.Col(2).x, rotation.Col(2).y, rotation.Col(2).z},
            {rotation.Col(1).x, rotation.Col(1).y, rotation.Col(1).z}
        );

        AK::SoundEngine::SetPosition(source->GetParentUID(), sourcePos);
    }

    AK::SoundEngine::RenderAudio();
    return UPDATE_CONTINUE;
}

void AudioModule::UnloadBanks()
{
    loadedAudio = false;
    AK::SoundEngine::UnloadBank(BANKNAME_MAIN, NULL);
    AK::SoundEngine::UnloadBank(BANKNAME_INIT, NULL);
}

bool AudioModule::ShutDown()
{
#ifndef AK_OPTIMIZED
    AK::Comm::Term();
#endif

    AK::MusicEngine::Term();
    AK::SoundEngine::Term();
    g_lowLevelIO.Term();
    if (AK::IAkStreamMgr::Get()) AK::IAkStreamMgr::Get()->Destroy();
    AK::MemoryMgr::Term();
    return true;
}

void AudioModule::AddAudioSource(AudioSourceComponent* newSource)
{
    sources.push_back(newSource);
    if (AK::SoundEngine::RegisterGameObj((AkGameObjectID)newSource->GetParentUID()) != AK_Success)
        GLOG("[ERROR] Audio source could not be registered");
}

void AudioModule::RemoveAudioSource(AudioSourceComponent* sourceToRemove)
{
    if (AK::SoundEngine::UnregisterGameObj((AkGameObjectID)sourceToRemove->GetParentUID()) != AK_Success)
        GLOG("[ERROR] Audio source could not be unregistered");

    const auto& it = std::find(sources.begin(), sources.end(), sourceToRemove);
    if (it != sources.end()) sources.erase(it);
}

bool AudioModule::AddAudioListener(AudioListenerComponent* newListener)
{
    if (listener) return false;

    listener                        = newListener;

    const AkGameObjectID listenerID = (AkGameObjectID)newListener->GetParentUID();
    if (AK::SoundEngine::RegisterGameObj(listenerID) != AK_Success)
        GLOG("[ERROR] Audio source could not be registered");
    AK::SoundEngine::SetDefaultListeners(&listenerID, 1);
    return true;
}

void AudioModule::RemoveAudioListener(AudioListenerComponent* listenerToRemove)
{
    if (listener != listenerToRemove) return;

    if (AK::SoundEngine::UnregisterGameObj((AkGameObjectID)listenerToRemove->GetParentUID()) != AK_Success)
        GLOG("[ERROR] Audio listener could not be unregistered");

    listener = nullptr;
}

void AudioModule::ParseEvents()
{
    const std::string metaPath = App->GetProjectModule()->GetLoadedProjectPath() + WINDOWS_BANKS_PATH + BANKMETA_MAIN;
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON(metaPath.c_str(), doc);

    if (!loaded)
    {
        GLOG("Failed to load bankMeta file: %s", metaPath);
        return;
    }

    if (!doc.HasMember("SoundBanksInfo") || !doc["SoundBanksInfo"].IsObject())
    {
        GLOG("Invalid bankMeta format: %s", metaPath);
        return;
    }

    rapidjson::Value& soundbankInfo = doc["SoundBanksInfo"];

    if (!soundbankInfo.HasMember("SoundBanks") || !soundbankInfo["SoundBanks"].IsArray())
    {
        GLOG("Invalid bankMeta format: %s", metaPath);
        return;
    }

    const rapidjson::Value& soundBank = soundbankInfo["SoundBanks"][0];

    if (soundBank.HasMember("Events") && soundBank["Events"].IsArray())
    {
        const rapidjson::Value& events = soundBank["Events"];
        for (rapidjson::SizeType i = 0; i < events.Size(); i++)
        {
            const rapidjson::Value& event = events[i];
            const std::string name        = event["Name"].GetString();
            const std::string id          = event["Id"].GetString();

            eventsMap.insert({name, static_cast<uint32_t>(std::stoul(id))});
        }
    }
    else
    {
        GLOG("Soundbank has no events: %s", metaPath);
    }
}