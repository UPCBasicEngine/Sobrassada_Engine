#version 460

layout(location = 0) out vec4 FragColor;

in vec4 vColor;

void main()
{
    FragColor = vColor;
}