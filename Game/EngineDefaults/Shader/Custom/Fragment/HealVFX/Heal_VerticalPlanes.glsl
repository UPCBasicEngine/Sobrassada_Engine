#version 460

#extension GL_ARB_bindless_texture : require

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
    const vec2 flippedUvs = vec2(uv.x, 1.0f - uv.y);
    const float subtractNode = flippedUvs.x - 0.5f;
    const float absoluteNode = abs(subtractNode);
    const float subtractNode2 = clamp(0.5f - absoluteNode, 0.0f, 1.0f);
    const float subtractNode3 = clamp(1.0f - flippedUvs.y, 0.0f, 1.0f);
    const float multiplyNode = subtractNode2 * subtractNode3;
    
    const float subtractNode4 = 25.0f - frameTimer;
    const float multiplyNode2 = subtractNode4 * 0.03f;
    const float bol = frameTimer > 14.0f ? 1.0f : 0.0f;
    const float multiplyNode3 = multiplyNode2 * bol;
    
    const float multiplyNode4 = clamp(multiplyNode * multiplyNode3, 0.0f, 1.0f);
    const float alpha = multiplyNode4 * diffColor.a;

    if (!isWireframe && isAlphaDiscard)
    {
        if(alpha < 0.05) discard;
    }

    // Ambient light
    vec3 BaseColor = diffColor.rgb;
    const vec3 emissive = vec3(0.468f, 1.0f, 0.195f);
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