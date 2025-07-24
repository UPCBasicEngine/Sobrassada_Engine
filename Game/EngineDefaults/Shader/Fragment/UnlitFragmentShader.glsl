#version 460

#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

in vec3 pos;
in vec2 uv0;
in vec3 normal;
in vec4 tangent;
flat in int instance_index;

out vec4 outColor;

uniform vec3 cameraPos;

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
    bool hasSpecular;
    bool hasMetallic;
};

readonly layout(std430, binding = 11) buffer Materials {
    Material materials[];
};

void main()
{
    Material mat = materials[instance_index];
    outColor = texture(sampler2D(mat.diffuseTex), uv0);
}