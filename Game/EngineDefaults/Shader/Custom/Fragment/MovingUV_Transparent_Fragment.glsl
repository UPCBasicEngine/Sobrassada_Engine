#version 460

#define PI 3.14159265359

layout(binding=0) uniform sampler2D diffuseTex;
layout(binding=1) uniform sampler2D metallicTex;
layout(binding=2) uniform sampler2D specularTex;
layout(binding=3) uniform sampler2D normalTex;

layout(location=4) uniform bool isWireframe;
layout(location=5) uniform bool isAlpha;
layout(location=6) uniform bool isMetallic;

layout(location=7) uniform float metallicFactor;
layout(location=8) uniform float roughnessFactor;
layout(location=9) uniform vec3 cameraPos;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

out vec4 fragColor;

mat3 CreateTBN()
{
    const vec3 T = normalize(vec3(tangent));
    const vec3 N = normalize(normal);
    const vec3 B = tangent.w * cross(N, T);
    return mat3(T, B, N);
}

void main()
{
    vec4 texColor = texture2D(diffuseTex, uv);

    const float alpha = texColor.a;

    if (!isWireframe && isAlpha)
    {
        if(alpha < 0.1) discard;
    }

    vec4 diffuse = vec4(pow(texColor.rgb, vec3(2.2f)), alpha);
    vec4 specular = vec4(1.f);

    if(isMetallic) specular = vec4(pow(texture2D(metallicTex, uv), vec4(2.2)));
    else specular = vec4(pow(texture2D(specularTex, uv), vec4(2.2)));

    vec4 position = vec4(pos, 1);
    vec4 Normal = vec4(normal, 0);
    
    specular.y = roughnessFactor * specular.y;
    specular.z = metallicFactor * specular.z;
    
    vec3 N = normalize(normal);
    vec3 normalTexSample = texture2D(normalTex, uv).xyz;

    // Retrive normal for normal map
    if (normalTexSample.r != 0 || normalTexSample.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (normalTexSample * 2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }
    vec4 finalNormal = vec4(N,0);

    const vec3 V = normalize(cameraPos - pos);
    const vec3 R = reflect(-V, N);
    const float NdotV = max(dot(N, V), 0.0001);

    fragColor = vec4(1.f);
}