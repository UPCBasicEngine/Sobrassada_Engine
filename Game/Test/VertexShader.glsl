#version 460
layout(location=0) in vec3 vertex_position;
layout(location=1) in vec4 vertex_tangent;
layout(location=2) in vec3 vertex_normal;
layout(location=3) in vec2 vertex_uv0;



layout(location=2) uniform mat4 model;
layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

out vec3 pos;
out vec3 normal;
out vec2 uv0;

void main()
{
	pos = vec3(model * vec4(vertex_position, 1.0));
    normal = vertex_normal;
    uv0 = vertex_uv0;
	gl_Position = projMatrix * viewMatrix * model * vec4(vertex_position, 1.0f);
}