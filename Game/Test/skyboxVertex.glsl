#version 460

layout(location = 0) in vec3 pos;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;

out vec3 texCoords;

void main()
{
    texCoords = pos;
    vec4 p = projection * vec4(mat3(view) * pos, 1.0);
    gl_Position = p.xyww;
} 