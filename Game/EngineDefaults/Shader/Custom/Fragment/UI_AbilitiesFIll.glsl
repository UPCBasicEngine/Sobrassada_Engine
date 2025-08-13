#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 emptyTexture;
layout(location = 5) uniform uvec2 filledTexture;

// Transition parameters
layout(location = 6) uniform float fillAmount;
//layout(location = 7) uniform float prevFillAmount;
//layout(location = 8) uniform float transitionTime;
//layout(location = 9) uniform float time;
//layout(location = 10) uniform float startTime;

// Wave parameters
layout(location = 7) uniform float waveAmplitude;
layout(location = 8) uniform float waveFrequency;
layout(location = 9) uniform float waveSpeed;

float easeOutElastic(float t) {
    float p = 0.7f;
    return pow(2.0, -10.0 * t) * sin((t - p / 4.0) * (6.28318) / p) + 1.0;
}

void main()
{
    vec4 empty = texture(sampler2D(emptyTexture), uv0);
    vec4 filled = texture(sampler2D(filledTexture), uv0);
    if (empty.a < 0.02f && filled.a < 0.02f) 
    {
       discard;
    }

    //float wave = sin(uv0.y * waveFrequency + time * waveSpeed) * waveAmplitude;
    //float liquidLevel = fillAmount + wave;

    if (uv0.y > 1.0f - fillAmount)
    {
        outColor = vec4(filled.r * inputColor.r, filled.g * inputColor.g, filled.b * inputColor.b, filled.a);
    }
    else
    {
        outColor = vec4(empty.r * inputColor.r, empty.g * inputColor.g, empty.b * inputColor.b, empty.a);
    }
}
