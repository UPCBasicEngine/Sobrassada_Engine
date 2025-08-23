#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 barTexture;

// Transition parameters
layout(location = 5) uniform float nextFillAmount;
layout(location = 6) uniform float prevFillAmount;
layout(location = 7) uniform float transitionTime;
layout(location = 8) uniform float time;
layout(location = 9) uniform float startTime;

// Wave parameters
layout(location = 10) uniform float waveAmplitude;
layout(location = 11) uniform float waveFrequency;
layout(location = 12) uniform float waveSpeed;

layout(location = 13) uniform float textureStart;
layout(location = 14) uniform float textureEnd;
layout(location = 15) uniform float textureWidth;

float easeOutElastic(float t) {
    float p = 0.7f;
    return pow(2.0, -10.0 * t) * sin((t - p / 4.0) * (6.28318) / p) + 1.0;
}

void main()
{
    vec4 sampled = texture(sampler2D(barTexture), uv0);
    if (sampled.a < 0.05) 
    {
       discard;
    }

    float startUv = textureStart / textureWidth;
    float endUv = textureEnd / textureWidth;
    float width = endUv - startUv;

    float t = clamp((time - startTime) / transitionTime, 0.0f, 1.0f);
    float fillAmount = mix(prevFillAmount, nextFillAmount, easeOutElastic(t));
    float scaledFill = startUv + (fillAmount * width);

    float wave = sin(uv0.y * waveFrequency + time * waveSpeed) * waveAmplitude;
    float liquidLevel = scaledFill + wave;    
    
    vec4 healthyColor = vec4(0.369f, 0.529f, 0.314f, 1.0f);
    vec4 damagedColor = vec4(1.0f, 0.1f, 0.1f,1.0f);

    vec4 finalColor = fillAmount <= 0.5f ? mix(damagedColor, healthyColor, fillAmount * 2.0f) : healthyColor;

    if (uv0.x <= liquidLevel)
    {
        outColor = vec4(finalColor.r * inputColor.r, finalColor.g * inputColor.g, finalColor.b * inputColor.b, sampled.a);
    }
    else
    {
        discard;
    }
        
}
