#version 460

#extension GL_ARB_bindless_texture : require

layout(location = 4) uniform uvec2 myTexture;
layout(location = 5) uniform float time;
layout(location = 6) uniform float fadeOutTime;
layout(location = 7) uniform float fadeOutDuration;
layout(location = 8) uniform bool isFadeOut;
layout(location = 9) uniform float fadeOutStart;

in vec2 uv0;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(sampler2D(myTexture), uv0);

    if (isFadeOut && time > fadeOutStart && time > fadeOutTime) 
    {
        const float fadeFactor = 1.0f - min(1.0f, ((time - fadeOutTime) / fadeOutDuration));
        texColor.a *= fadeFactor;
    }
    
    if (texColor.a < 0.01f) {
        texColor.a;
    }

    fragColor = texColor;
}