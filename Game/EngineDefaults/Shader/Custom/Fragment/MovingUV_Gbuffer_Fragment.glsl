#version 460

#extension GL_ARB_bindless_texture : require

layout(location=4) uniform bool isWireframe;
layout(location=5) uniform bool isAlphaDiscard;
layout(location=16) uniform int baseIndex;

layout(location = 0)out vec4 gDiffuse;
layout(location = 1)out vec4 gSpecular;
layout(location = 2)out vec4 gPosition;
layout(location = 3)out vec4 gNormal;
layout(location = 4)out vec4 gEmissive;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

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
    const Material mat = materials[baseIndex];

    vec4 texColor = pow(texture(sampler2D(mat.diffuseTex), uv), vec4(2.2f));

    const float alpha = texColor.a;

    if (!isWireframe && isAlphaDiscard)
    {
        if(alpha < 0.1) discard;
    }

    gDiffuse = vec4(texColor.rgb, alpha);

    
    
    gPosition = vec4(pos, 1);
    gNormal = vec4(normal, 0);
    
    if(mat.hasMetallic == 0)
    {
        gSpecular = pow(texture(sampler2D(mat.metallicTex), uv), vec4(2.2));
        gSpecular.y = mat.roughnessFactor * gSpecular.y;
        gSpecular.z = mat.metallicFactor * gSpecular.z;
    }
    else
    {
        gSpecular = pow(texture(sampler2D(mat.specularTex), uv), vec4(2.2));
    }
    
    vec3 N = normalize(normal);
    vec3 normalTexSample = texture(sampler2D(mat.normalTex), uv).xyz;

    // Retrive normal for normal map
    if (normalTexSample.r != 0 || normalTexSample.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (normalTexSample * 2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }
    gNormal = vec4(N,0);

    vec3 emissiveColor = pow(texture(sampler2D(mat.emmisiveTex), uv).rgb, vec3(2.2f));
    gEmissive = vec4(emissiveColor, 1.0);
}