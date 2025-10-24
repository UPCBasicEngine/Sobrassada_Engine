#version 460

#extension GL_ARB_bindless_texture : require

layout(binding=0) uniform sampler2D noiseTexture;

layout(location=3) uniform float dissolveAmmount;
layout(location=4) uniform int baseIndex;

struct Material
{
    vec4 diffColor;
    vec3 specColor;
    float shininess;
    bool shininessInAlpha;
    float metallicFactor;
    float roughnessFactor;
    uvec2 diffuseTex;
    uvec2 specularTex;
    uvec2 metallicTex;
    uvec2 normalTex;
    int hasSpecular;
    int hasMetallic;
    uvec2 emmisiveTex;
    uvec2 occlusionTex;
    float emissiveIntensity;
    float padding;
};

readonly layout(std430, binding = 11) buffer Materials {
    Material materials[];
};

in vec2 uv;

out vec4 fragColor;

void main()
{
    const Material mat = materials[baseIndex];

    vec4 texColor = texture(sampler2D(mat.diffuseTex), uv);

    float noiseSample = texture2D(noiseTexture, uv).r;

    float isVisible = (noiseSample * 0.99) - dissolveAmmount;
    
    if(isVisible < 0) discard;

    fragColor = texColor;
}