#version 460

layout(location = 0) in vec3 vertex_position;

uniform mat4 model;

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

out vec4 clipping;

void main(){
    clipping = projMatrix * viewMatrix * model * vec4(vertex_position, 1.0);
    gl_Position = clipping;
}