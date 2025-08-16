#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 emptyTexture;
layout(location = 5) uniform uvec2 filledTexture;

// Transition parameters
layout(location = 6) uniform float fillAmount;
layout(location = 7) uniform float time;

// Wave parameters
layout(location = 8) uniform float waveAmplitude;
layout(location = 9) uniform float waveFrequency;
layout(location = 10) uniform float waveSpeed;

void main()
{
    vec4 empty = texture(sampler2D(emptyTexture), uv0);
    vec4 filled = texture(sampler2D(filledTexture), uv0);
    if (empty.a < 0.02f && filled.a < 0.02f) 
    {
       discard;
    }

    float wave = sin(uv0.x * waveFrequency + time * waveSpeed) * waveAmplitude;
    float liquidLevel = fillAmount + wave;

    if (uv0.y > 1.0f - liquidLevel)
    {
        outColor = vec4(filled.r * inputColor.r, filled.g * inputColor.g, filled.b * inputColor.b, filled.a);
    }
    else
    {
        outColor = vec4(empty.r * inputColor.r, empty.g * inputColor.g, empty.b * inputColor.b, empty.a);
    }
}
