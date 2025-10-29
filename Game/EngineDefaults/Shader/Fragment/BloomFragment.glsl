#version 460

layout(binding = 0) uniform sampler2D hdrScene;
layout(binding = 1) uniform sampler2D bloomBlur;
uniform float bloomIntensity;

in vec2 uv0;

out vec4 fragColor;

void main()
{
    vec3 hdrColor = texture(hdrScene, uv0).rgb;
    vec3 bloom = texture(bloomBlur, uv0).rgb;

    vec3 result = hdrColor + bloom * bloomIntensity;

    fragColor = vec4(result, 1.0);
}