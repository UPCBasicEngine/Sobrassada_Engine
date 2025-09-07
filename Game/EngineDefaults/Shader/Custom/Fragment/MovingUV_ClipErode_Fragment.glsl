#version 460
#extension GL_ARB_bindless_texture : require

layout(location = 3)  uniform vec2  uUVOffset;      
layout(location = 4)  uniform bool  isWireframe;    
layout(location = 5)  uniform bool  isAlphaDiscard;
layout(location = 6)  uniform vec3  cameraPos;      

layout(location = 7)  uniform float uErode;         
layout(location = 8)  uniform float uEdgeFeather; 
layout(location = 11) uniform vec2  uClipMin;     
layout(location = 12) uniform vec2  uClipMax;      

in vec2 uv;

out vec4 fragColor;

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

float rectFeather(vec2 t, vec2 minUV, vec2 maxUV, float feather)
{
    vec2 inMin = smoothstep(minUV,               minUV + vec2(feather), t);
    vec2 inMax = 1.0    - smoothstep(maxUV - vec2(feather), maxUV,      t);
    return inMin.x * inMin.y * inMax.x * inMax.y;
}

void main()
{
    vec2 tuv = uv + uUVOffset;

    vec4 tex = texture(sampler2D(diffuseTex), tuv);

    float alpha = tex.a * diffColor.a;

    alpha = clamp(alpha - uErode, 0.0, 1.0);

    float mask = rectFeather(tuv, uClipMin, uClipMax, uEdgeFeather);
    alpha *= mask;

    if (!isWireframe && isAlphaDiscard && alpha <= 0.0) discard;

    fragColor = vec4(tex.rgb * diffColor.rgb, alpha);
}
