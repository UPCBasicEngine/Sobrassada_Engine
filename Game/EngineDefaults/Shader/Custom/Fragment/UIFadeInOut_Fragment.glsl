#version 460

#extension GL_ARB_bindless_texture : require

layout(location = 3) uniform uvec2 myTexture;
layout(location = 4) uniform float timer;

layout(location = 5) uniform bool isFadingIn;
layout(location = 6) uniform float fadeInStart;
layout(location = 7) uniform float fadeInDuration;
layout(location = 8) uniform float fadeInOpacity;

layout(location = 9) uniform bool isFadingOut;
layout(location = 10) uniform float fadeOutStart;
layout(location = 11) uniform float fadeOutDuration;
layout(location = 12) uniform float fadeOutOpacity;

layout(location = 13) uniform bool isVisible;

in vec2 uv0;    

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(sampler2D(myTexture), uv0);

    float alpha = isVisible ? fadeInOpacity : fadeOutOpacity;

    // Should never be both active at same time. If it happens it's a logic problem
    if (isFadingIn) 
    {
        const float fadeFactor = max(fadeOutOpacity, min(fadeInOpacity, ((timer - fadeInStart) / fadeInDuration)));
        alpha = fadeFactor;
    }
    if (isFadingOut) 
    {
        const float fadeFactor = max(fadeOutOpacity, fadeInOpacity - min(fadeInOpacity, ((timer - fadeOutStart) / fadeOutDuration)));
        alpha = fadeFactor;
    }
    
    if (alpha < 0.01f) {
        discard;
    }

    texColor.a = alpha;

    fragColor = texColor;
}