#version 460

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec2 vertexUV;
layout(location=2) in vec3 billboardCenter;
layout(location=3) in vec2 tileOffset;

layout(location=0) uniform vec3 cameraRightVector;
layout(location=1) uniform vec3 cameraUpVector;
layout(location=2) uniform vec2 billboardSize;
layout(location=3) uniform mat4 VP;
layout(location=4) uniform vec2 tileSize;
layout(location=5) uniform float currentFrame;

out vec2 uv;
out vec2 uvNext;
flat out float blendFactor;

void main()
{

    float X = trunc(mod(currentFrame + tileOffset.x, tileSize.x));
    float Y  = trunc((currentFrame + tileOffset.y) / tileSize.y);
    Y = tileSize.y - 1 - Y;

    float XNext = trunc(mod(currentFrame + tileOffset.x + 1.f, tileSize.x));
    float YNext  = trunc((currentFrame + tileOffset.y + 1.f) / tileSize.y);
    YNext = tileSize.y - 1 - YNext;

    float U = mix(X, X+1, vertexUV.x) / tileSize.x;
    float V = mix(Y, Y+1, vertexUV.y) / tileSize.y;

    float UN = mix(X, X+1, vertexUV.x) / tileSize.x;
    float VN = mix(Y, Y+1, vertexUV.y) / tileSize.y;

    blendFactor = currentFrame - trunc(currentFrame);

    uv = vec2(U,V);
    uvNext = vec2(UN, VN);

    gl_Position = VP * vec4(billboardCenter + cameraRightVector * vertexPosition.x * billboardSize.x
    + cameraUpVector * vertexPosition.y * billboardSize.y, 1.f);
}