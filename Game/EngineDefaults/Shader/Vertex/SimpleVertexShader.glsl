#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

layout(location = 4) uniform mat4 model;

out vec4 vColor;

void main()
{
    gl_Position = projMatrix * viewMatrix * model * vec4(aPosition, 1.0);
    vColor = aColor;
}