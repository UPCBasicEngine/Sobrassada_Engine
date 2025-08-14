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
    const float subtractNode = (frameTimer * 0.15f) - 2.0f;
    const float powerNode = pow(subtractNode, 0.35f);
    const float multiplyAddNode = (powerNode * 2.91f) - 2.21f;
    const vec2 combineNode = vec2(0.0f, multiplyAddNode);
 
    const vec2 flippedUvs = vec2(uv.x, (1.0f - uv.y));
    const vec2 addNode = flippedUvs + combineNode;
    const vec2 combineNode2 = vec2(clamp(addNode.y, 0.0f, 1.0f), addNode.x);
    const vec4 textureNode = texture(sampler2D(diffuseTex), combineNode2);
 
    const vec4 scaleNode = textureNode * flippedUvs.y;
    const float alpha = scaleNode.a * diffColor.a;

    if (!isWireframe && isAlphaDiscard)
    {
        if(alpha < 0.05) discard;
    }

    vec3 BaseColor = diffColor.rgb * texColor.rgb;
    const vec3 emissive = vec3(0.276f, 1.0f, 0.009f);
    BaseColor += emissive.rgb * 2;
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