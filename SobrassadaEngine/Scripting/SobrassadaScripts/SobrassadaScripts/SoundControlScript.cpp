#include "pch.h"

#include "SoundControlScript.h"
#include "CuChulainn.h"
#include "Standalone/CharacterControllerComponent.h"

SoundControlScript::SoundControlScript(GameObject* parent) : Script(parent)
{
	fields.push_back({ "MinDistanceToPlayer", InspectorField::FieldType::Float, &minDistanceToPlayer, 0.0f, 100.0f });

};

SoundControlScript::DynamicSoundModifier() {

};

void SoundControlScript::Update(float deltaTime)
{
    if (!character) return;

    const float distance = character->GetLastPosition().Distance(finalPosition);

    // Disable when far away (for performance)
    isActive = (distance <= minDistanceToPlayer * 2);
    if (!isActive) return;
};