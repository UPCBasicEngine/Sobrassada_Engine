#version 460
layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec4 vertexTangent;
layout(location=2) in vec3 vertexNormal;
layout(location=3) in vec2 vertexUV;
layout(location=4) in ivec4 vertex_joint;
layout(location=5) in vec4 vertex_weights;

layout(location=0) uniform mat4 proj;
layout(location=1) uniform mat4 view;
layout(location=2) uniform mat4 model;

layout(location=9) uniform bool hasBones;
layout(location=10) uniform uint boneIndex;

readonly layout(std430, row_major, binding = 12) buffer Bones {
    mat4 palettes[];
};

out vec2 uv;
out vec3 normal;

void main()
{
    vec3 pos;
    if (hasBones) 
    {
        mat4 skin    = palettes[boneIndex + vertex_joint[0]] * vertex_weights[0] + palettes[boneIndex + vertex_joint[1]] * vertex_weights[1] +             
                       palettes[boneIndex + vertex_joint[2]] * vertex_weights[2] + palettes[boneIndex + vertex_joint[3]] * vertex_weights[3];
        pos          = (skin * vec4(vertexPosition, 1.0)).xyz;
    }
    else pos = vec3(model * vec4(vertexPosition, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    normal = normalMatrix * vertexNormal;
    uv = vertexUV;
    gl_Position = proj * view * vec4(pos, 1.0f);
}