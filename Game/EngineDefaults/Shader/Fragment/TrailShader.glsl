#version 460

uniform bool uHasTexture;
uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;

in vec4 vColor;
in vec2 vUV;

void main()
{
    if (uHasTexture)
        FragColor = texture(uTexture, vUV) * vColor;
    else
        FragColor = vColor;
}