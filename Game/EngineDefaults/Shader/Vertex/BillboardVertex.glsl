#version 460

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec2 vertexUV;
layout(location=2) in vec3 billboardCenter;

layout(location=0) uniform vec3 cameraRightVector;
layout(location=1) uniform vec3 cameraUpVector;
layout(location=2) uniform vec2 billboardSize;
layout(location=3) uniform mat4 VP;
layout(location=4) uniform vec4 uvRange;

out vec2 uv;

void main()
{
    uv = vec2(mix(uvRange.x, uvRange.y, vertexUV.x), mix(uvRange.z, uvRange.w, vertexUV.y));
    gl_Position = VP * vec4(billboardCenter + cameraRightVector * vertexPosition.x * billboardSize.x
    + cameraUpVector * vertexPosition.y * billboardSize.y, 1.f);
}