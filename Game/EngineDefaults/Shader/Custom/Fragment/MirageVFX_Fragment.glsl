#version 460

#extension GL_ARB_bindless_texture : require

layout(location = 1) uniform mat4 view;
layout(location = 7) uniform float animationTimer;

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

in vec2 uv;

out vec4 fragColor;

vec3 colorRamp(float x){
    float thresholds[4] = float[4](0.02, 0.258, 0.433, 0.649);
    vec3 colors[4] = vec3[4](
        vec3(0, 0, 0), 
        vec3(0.188, 0.357, 0.733), 
        vec3(0.153, 0.941, 0.957), 
        vec3(1, 1, 1)
    );
    if(x < thresholds[0]) return colors[0];
    if(x >= thresholds[3]) return colors[3];

    int i;
    for(i = 0; i < 4; ++i){
        if(x < thresholds[i]) break;
    }
    float numerator = x - thresholds[i-1];
    float denominator = thresholds[i] - thresholds[i-1];
    float r = numerator / denominator;
    return colors[i-1] * (1.0 - r) + colors[i] * r;
}

void main()
{
    vec2 texcoord_read = vec2(uv.y, animationTimer);
    float read = texture(sampler2D(diffuseTex), texcoord_read).x;
    float value = uv.x * (read + 0.39);

    fragColor = vec4(colorRamp(value), 1 - value);
}