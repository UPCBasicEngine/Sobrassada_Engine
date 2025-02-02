#pragma once
#include <vector>
#include <iostream>
#include "Math/float3.h"
#include "Math/float4.h"

class Material
{
  public:
    std::string name;

    bool hasDiffuseMap = false;
    std::vector<double>  diffuseColor = {0.137f, 0.263f, 0.424f};
    int diffuseMap = 0;
    float diffuseIntensity = 1.0f;

    bool hasSpecularMap = false;
    std::vector<double> specularColor = {1.0f, 1.0f, 0.423f};
    int specularMap = 0;
    float specularIntensity = 1.0f;
    float shininess = 300.00f;
    bool hasShininessInAlpha = false;
    
   

    bool isPbrSpecularGlossines = false;
    std::vector<double> diffuseFactor = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<double> specularFactor = {1.0, 1.0f, 1.0f};
    double glossinessFactor = 1.0f;
    int specularGlossinesTexture = 0;
    float specularGlossinessIntensity  = 1.0f;

};
