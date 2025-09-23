#version 460

layout(location=3) uniform vec3 targetColor;

out vec4 fragColor;

void main()
{
    fragColor = vec4(targetColor, 1.0f);
}