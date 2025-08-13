#version 460

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec4 vertexTangent;
layout(location=2) in vec3 vertexNormal;
layout(location=3) in vec2 vertexUV;
layout(location=4) in ivec4 vertexJoint;
layout(location=5) in vec4 vertexWeights;

layout(location=0) uniform mat4 proj;
layout(location=1) uniform mat4 view;
layout(location=2) uniform mat4 model;
layout(location=3) uniform vec2 uvOffset;

layout(location=9) uniform bool hasBones;
layout(location=10) uniform uint boneIndex;

readonly layout(std430, row_major, binding = 12) buffer Bones {
    mat4 palettes[];
};

out vec2 uv;

void main()
{
    uv = vertexUV + uvOffset;
    vec3 pos = vec3(model * vec4(vertexPosition, 1.0));

    if (hasBones) 
    {
        mat4 skin    = palettes[boneIndex + vertexJoint[0]] * vertexWeights[0] + palettes[boneIndex + vertexJoint[1]] * vertexWeights[1] +             
                       palettes[boneIndex + vertexJoint[2]] * vertexWeights[2] + palettes[boneIndex + vertexJoint[3]] * vertexWeights[3];
        pos          = (skin * vec4(vertexPosition, 1.0)).xyz;
    }

    gl_Position = proj * view * vec4(pos, 1.0f);
}