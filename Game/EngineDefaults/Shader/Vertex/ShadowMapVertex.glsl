#version 460

layout(location=0) in vec3 vertex_position;
layout(location=4) in ivec4 vertex_joint;
layout(location=5) in vec4 vertex_weights;

layout(location=7) uniform bool hasBones;

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

readonly layout(std430, row_major, binding = 10) buffer Transforms {
    mat4 models[];
};

readonly layout(std430, row_major, binding = 12) buffer Bones {
    mat4 palettes[];
};

readonly layout(std430, row_major, binding = 13) buffer AccBones {
    uint bonesIndex[];
};

void main()
{
    vec4 pos;

    if (hasBones) 
    {
        uint boneIndex = bonesIndex[gl_BaseInstance];
        mat4 skin    = palettes[boneIndex + vertex_joint[0]] * vertex_weights[0] + palettes[boneIndex + vertex_joint[1]] * vertex_weights[1] +             
                       palettes[boneIndex + vertex_joint[2]] * vertex_weights[2] + palettes[boneIndex + vertex_joint[3]] * vertex_weights[3];
        pos          = (skin * vec4(vertex_position, 1.0));
    } 
    else 
    {
        mat4 model = models[gl_BaseInstance];
        pos = model * vec4(vertex_position, 1.0);
    }

    gl_Position = projMatrix * viewMatrix * pos; 
}