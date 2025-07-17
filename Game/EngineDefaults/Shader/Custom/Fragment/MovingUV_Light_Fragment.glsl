#version 460

layout(binding=0) uniform sampler2D diffuseTex;
layout(binding=1) uniform sampler2D metallicTex;
layout(binding=2) uniform sampler2D specularTex;
layout(binding=3) uniform sampler2D normalTex;

layout(location=4) uniform bool isWireframe;
layout(location=5) uniform bool isAlpha;
layout(location=6) uniform bool isMetallic;

layout(location=7) uniform float metallicFactor;
layout(location=8) uniform float roughnessFactor;

layout(location = 0)out vec4 gDiffuse;
layout(location = 1)out vec4 gSpecular;
layout(location = 2)out vec4 gPosition;
layout(location = 3)out vec4 gNormal;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

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

    gDiffuse = vec4(pow(texColor.rgb, vec3(2.2f)), alpha);

    if(isMetallic) gSpecular = vec4(pow(texture2D(metallicTex, uv), vec4(2.2)));
    else gSpecular = vec4(pow(texture2D(specularTex, uv), vec4(2.2)));

    gPosition = vec4(pos, 1);
    gNormal = vec4(normal, 0);
    
    gSpecular.y = roughnessFactor * gSpecular.y;
    gSpecular.z = metallicFactor * gSpecular.z;
    
    vec3 N = normalize(normal);
    vec3 normalTexSample = texture2D(normalTex, uv).xyz;

    // Retrive normal for normal map
    if (normalTexSample.r != 0 || normalTexSample.g != 0) {
        const mat3 space = CreateTBN();
        const vec3 texNormal = (normalTexSample * 2.0-1.0);
        const vec3 final_normal = space * texNormal;
        N = normalize(final_normal);
    }
    gNormal = vec4(N,0);;
}