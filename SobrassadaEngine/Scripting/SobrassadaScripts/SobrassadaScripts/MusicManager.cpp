
#include "pch.h"
#include "MusicManager.h"
#include "GameObject.h"

MusicManager::MusicManager(GameObject* parent): Script(parent){}

void MusicManager::OnPlayerRespawn() const
{
    for (UID childUID: parent->GetChildren())
    {
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(true);
    }
}


