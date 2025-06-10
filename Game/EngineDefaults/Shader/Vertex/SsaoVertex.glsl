#version 460

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = texCoords;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}