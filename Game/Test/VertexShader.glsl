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
out vec4 tangent;
out vec3 fragViewPos;

void main()
{
	pos = vec3(model * vec4(vertex_position, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    normal = normalMatrix * vertex_normal;
    uv0 = vertex_uv0;
    tangent = vec4(normalMatrix * vertex_tangent.xyz, vertex_tangent.w);

    //Camera position in World Space
    fragViewPos = vec3(inverse(viewMatrix)[3]);

	gl_Position = projMatrix * viewMatrix * model * vec4(vertex_position, 1.0f);
}