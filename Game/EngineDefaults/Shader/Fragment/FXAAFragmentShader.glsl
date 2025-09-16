#version 460

in vec2 uv0;

layout(binding=0) uniform sampler2D lastDrawTexture;

out vec4 fragColor;

void main()
{
    fragColor = texture(lastDrawTexture, uv0);
}