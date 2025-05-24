#version 460

layout(location = 0) in vec3 vertex_position;

struct Models {
    mat4 model;
    mat4 invModel;
};

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

readonly layout(std430, row_major, binding = 1) buffer Materials {
    Models modelMatrices[];
};

out mat4 vInvModel;
out vec4 clipping;

void main(){
    const Models models = modelMatrices[gl_BaseInstance];
    vInvModel = models.invModel;
    clipping = projMatrix * viewMatrix * models.model * vec4(vertex_position, 1.0);
    gl_Position = clipping;
}