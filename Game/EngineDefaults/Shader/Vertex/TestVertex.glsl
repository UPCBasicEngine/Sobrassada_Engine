#version 460

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec2 vertexUV;

layout(location=0) uniform mat4 proj;
layout(location=1) uniform mat4 view;
layout(location=2) uniform mat4 model;
layout(location=3) uniform vec2 uvOffset;

out vec2 uv;

void main()
{
    uv = vertexUV + uvOffset;
    gl_Position =  gl_Position = proj * view * model * vec4(vertexPosition, 1.0f);
}