#pragma once

#include "HashString.h"

class ParticleSystem
{
  public:
    ParticleSystem();
    ParticleSystem(int x);
    ~ParticleSystem();

  private:
    HashString particleSystemTag = HashString("");
};
