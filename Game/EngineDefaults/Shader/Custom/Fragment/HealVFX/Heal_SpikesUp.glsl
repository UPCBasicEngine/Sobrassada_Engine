#version 460

#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

layout(location=4) uniform bool isWireframe;
layout(location=5) uniform bool isAlphaDiscard;
layout(location=6) uniform vec3 cameraPos;
layout(location=7) uniform float frameTimer;

in vec3 pos;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

out vec4 fragColor;

// UBOs
layout(std140, binding = 6) uniform Material
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

void main()
{
    vec4 texColor = pow(texture(sampler2D(diffuseTex), uv), vec4(2.2f));

    // Blender nodes recreation
    float subtractNode = frameTimer - 13.82f;
    float powerNode = pow(1.81f, subtractNode);
    float subtractNode2 = powerNode - 1.0f;
    float divideNode = clamp(6.87f / subtractNode2, 0.0f, 10000.0f); 
  
    vec2 flippedUvs = vec2(uv.x, (1.0f - uv.y));
    vec2 subtractNode3 = flippedUvs - vec2(0.5f, 0.5f);
    vec2 scaleNode = subtractNode3 * divideNode;
    vec2 addNode = scaleNode + vec2(0.5f, 0.5f);
    vec4 texNode = texture(sampler2D(diffuseTex), clamp(addNode, 0.0f, 1.0f));

    float alpha = texNode.a * diffColor.a;

    if (!isWireframe && isAlphaDiscard)
    {
        if(alpha < 0.05) discard;
    }

    vec3 BaseColor = diffColor.rgb * texColor.rgb;
    vec4 emissive = texNode;
    BaseColor += emissive.rgb;
    const vec3 ldr = BaseColor / (BaseColor + vec3(1.0));

    if (isWireframe)
    {
        fragColor = vec4(ldr, 1.0);
    }
    else 
    {
        fragColor = vec4(ldr * alpha, 1.0f);
    } 
}