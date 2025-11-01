#include "pch.h"

#include "Application.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "Mirage.h"
#include "MirageBossDash.h"
#include "MirageVFX.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone\MeshComponent.h"
#include "Wwise_IDs.h"

Mirage::Mirage(GameObject* parent) : Script(parent)
{

    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 20.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 20.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back({"Weight Order", InspectorField::FieldType::Int, &weightOrder, 0, 100});
}

// 1 ferdiad 2 endpoint 3 border 4 fire 5 arrow
//
// this is so ugly but it works
bool Mirage::Init()
{
    Scene* scene                = AppEngine->GetSceneModule()->GetScene();

    state                       = MirageState::Sleeping;
    stateTimer                  = 0.0f;
    meshComponent               = parent->GetComponent<MeshComponent*>();
    std::vector<UID> children   = parent->GetChildren();

    GameObject* firstChild      = scene->GetGameObjectByUID(children[0]);

    ScriptComponent* scriptComp = firstChild->GetComponent<ScriptComponent*>();

    GameObject* secondChild     = scene->GetGameObjectByUID(children[1]);

    GameObject* thirdChild      = scene->GetGameObjectByUID(children[2]);
    GameObject* fourthChild     = scene->GetGameObjectByUID(children[3]);

    std::vector<UID> childChild = thirdChild->GetChildren();

    GameObject* thirdChildChild = scene->GetGameObjectByUID(childChild[0]);

    mirageBorder                = thirdChild->GetComponent<MeshComponent*>();
    mirageDisableComponent2     = fourthChild->GetComponent<MeshComponent*>();
    mirageArrow                 = thirdChildChild->GetComponent<MeshComponent*>();

    bossDash                    = scriptComp->GetScriptByType<MirageBossDash>();
    endPoint                    = secondChild->GetLocalTransform().TranslatePart();

    mirageFireComponent         = fourthChild->GetComponent<ShaderScriptComponent*>();
    firescript                  = mirageFireComponent->GetScriptByType<MirageVFX>();

    audio                       = parent->GetComponent<AudioSourceComponent*>();

    bossDash->setEndPoint(endPoint);

    matMirageArrowBlue  = AppEngine->GetLibraryModule()->GetMaterialUID("m_mirage_plane_arrow_2");
    matMirageBorderBlue = AppEngine->GetLibraryModule()->GetMaterialUID("m_mirage_plane_border_2");

    if (matMirageArrowBlue == INVALID_UID) GLOG("Mirage arrow materials not found in library!");

    if (matMirageBorderBlue == INVALID_UID) GLOG("Mirage border materials not found in library!");

    parent->SetEnabled(false);

    return true;
}

void Mirage::Update(float deltaTime)
{
    switch (state)
    {
    case MirageState::Sleeping:
    {
        parent->SetEnabled(true);
        firescript->SetAllColors(float3(0.984f, 0.690f, 0.231f)); // GOLD
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_PREPAREMIRAGE);
        mirageDisableComponent2->SetEnabled(false);
        state      = MirageState::Warning;
        stateTimer = 0.0f;
        GLOG("Calling gameobject");

        break;
    }

    case MirageState::Warning:
    {
        stateTimer += deltaTime;
        GLOG("Activating gameobject");

        if (stateTimer >= warningDelay)
        {
            state      = MirageState::Damaging;
            stateTimer = 0.0f;

            if (bossDash)
            {
                bossDash->setState(BossDashStates::OverheadStrike);
                bossDash->setAction(BossDashActions::Prepare);
                bossDash->setStateBool(true);
            }
        }
        break;
    }

    case MirageState::Damaging:
    {
        stateTimer += deltaTime;

        firescript->SetAllColors(float3(0.188f, 0.357f, 0.733f)); // BLUE

        mirageBorder->AddMaterial(matMirageBorderBlue);
        mirageArrow->AddMaterial(matMirageArrowBlue);

        if (stateTimer >= 1 && !dashdone)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_DASHATTACK_02);
            dashdone = true;
        }
        if (stateTimer >= damageDuration)
        {
            state = MirageState::Sleeping;
            parent->SetEnabled(false);
        }
        break;
    }
    }
}