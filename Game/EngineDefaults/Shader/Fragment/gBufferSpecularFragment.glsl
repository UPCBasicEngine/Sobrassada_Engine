#version 460

#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

layout(location = 0)out vec4 gDiffuse;
layout(location = 1)out vec4 gSpecular;
layout(location = 2)out vec4 gPosition;
layout(location = 3)out vec4 gNormal;
layout(location = 4)out vec4 gEmissive;

in vec3 pos;
in vec2 uv0;
in vec3 normal;
in vec4 tangent;
flat in int instance_index;

uniform bool isWireframe;
uniform bool isAlpha;

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
    uvec2 padding;
};

readonly layout(std430, binding = 11) buffer Materials {
    Material materials[];
};

mat3 CreateTBN()
{
    const vec3 T = normalize(vec3(tangent));
    const vec3 N = normalize(normal);
    const vec3 B = tangent.w * cross(N, T);
    return mat3(T, B, N);
}

void main()
{
    const Material mat = materials[instance_index];

    vec4 texColor = texture(sampler2D(mat.diffuseTex), uv0);
    const float alpha = texColor.a * mat.diffColor.a;

    if (!isWireframe && isAlpha)
    {
        if(alpha < 0.1) discard;
    }

    gDiffuse = vec4(pow(texColor.rgb, vec3(2.2f)), alpha);
    
    if(mat.hasSpecular == 1) gSpecular = vec4(pow(texture(sampler2D(mat.specularTex), uv0), vec4(2.2)));
    else gSpecular = vec4(1);
    gPosition = vec4(pos,0);
    
    vec3 N = normalize(normal);
    // Retrive normal for normal map
    if (mat.normalTex.r != 0 || mat.normalTex.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (texture(sampler2D(mat.normalTex), uv0).xyz*2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }
    
    gNormal = vec4(N,0);

    vec3 emissiveColor = pow(texture(sampler2D(mat.emmisiveTex), uv0).rgb, vec3(2.2f));
    gEmissive = vec4(emissiveColor, 1.0);
}