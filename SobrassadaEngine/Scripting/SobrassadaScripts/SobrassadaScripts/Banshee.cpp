#include "pch.h"

#include "Banshee.h"

#include "GameObject.h"

Banshee::Banshee(GameObject* parent)
    : Character(
          parent,
          1, // Max Health
          2, // Damage
          3, // Attack Duration
          4, // Attack Cooldown
          5, // Attack Range
          5, // AI Aggro Range
          5, // AI Chase Range
          CharacterType::Banshee
      )
{
}

bool Banshee::Init()
{
    return true;
}

void Banshee::Update(float deltaTime)
{
}

void Banshee::OnDeath()
{
}

void Banshee::OnDamageTaken(int amount)
{
}

void Banshee::PerformAttack()
{
}

void Banshee::HandleState(float deltaTime)
{
}

void Banshee::ChasePlayer()
{
}

void Banshee::Flee()
{
}