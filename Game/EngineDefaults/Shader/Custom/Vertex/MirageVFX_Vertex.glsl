#version 460
layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec4 vertexTangent;
layout(location=2) in vec3 vertexNormal;
layout(location=3) in vec2 vertexUV;

layout(location=0) uniform mat4 proj;
layout(location=1) uniform mat4 view;
layout(location=2) uniform mat4 model;

out vec2 uv;
out vec3 normal;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    normal = normalMatrix * vertexNormal;
    vec3 pos = vec3(model * vec4(vertexPosition, 1.0));
    uv = vertexUV;
    gl_Position = proj * view * vec4(pos, 1.0f);
}