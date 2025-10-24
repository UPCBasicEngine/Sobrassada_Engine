#version 460

#extension GL_ARB_bindless_texture : require

layout(binding=0) uniform sampler2D noiseTexture;

layout(location=3) uniform float dissolveAmmount;
layout(location=4) uniform int baseIndex;
layout(location=5) uniform float glowRange;
layout(location=6) uniform float glowFallof;
layout(location=7) uniform vec4 glowColor;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

layout(location = 0)out vec4 gDiffuse;
layout(location = 1)out vec4 gSpecular;
layout(location = 2)out vec4 gPosition;
layout(location = 3)out vec4 gNormal;
layout(location = 4)out vec4 gEmissive;

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


mat3 CreateTBN()
{
    const vec3 T = normalize(vec3(tangent));
    const vec3 N = normalize(normal);
    const vec3 B = tangent.w * cross(N, T);
    return mat3(T, B, N);
}

void main()
{
    float noiseSample = texture2D(noiseTexture, uv).r;
    float isVisible = (noiseSample * 0.99) - dissolveAmmount;
    
    if(isVisible < 0) discard;

    const Material mat = materials[baseIndex];
    vec4 texColor = texture(sampler2D(mat.diffuseTex), uv);
    const float alpha = texColor.a * mat.diffColor.a;


    vec3 baseColor = materials[baseIndex].diffColor.rgb * texColor.rgb;
    gDiffuse = vec4(pow(baseColor, vec3(2.2f)), alpha);
    if(mat.hasMetallic == 1) gSpecular = vec4(pow(texture(sampler2D(mat.metallicTex), uv), vec4(2.2)));
    else gSpecular = vec4(1);
    gPosition = vec4(pos, 1);
    gNormal = vec4(normal, 0);

    gSpecular.y = mat.roughnessFactor * gSpecular.y;
    gSpecular.z = mat.metallicFactor * gSpecular.z;

    vec3 N = normalize(normal);
    // Retrive normal for normal map
    if (mat.normalTex.r != 0 || mat.normalTex.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (texture(sampler2D(mat.normalTex), uv).xyz*2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }
    gNormal = vec4(N,0);
    vec3 emissiveColor = pow(texture(sampler2D(mat.emmisiveTex), uv).rgb, vec3(2.2f)) * vec3(mat.emissiveIntensity);

    float isGlowing = smoothstep(glowRange + glowFallof, glowRange, isVisible);
    vec4 glowEmissive = isGlowing * glowColor;

    gEmissive = vec4(emissiveColor + glowEmissive.rgb, 1.0);
}