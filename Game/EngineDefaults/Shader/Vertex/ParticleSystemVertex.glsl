#version 460

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec2 vertexUV;
layout(location=2) in vec3 billboardCenter;
layout(location=3) in vec2 tileOffset;
layout(location=4) in vec4 particleColor;
layout(location=5) in vec2 billboardSize;
layout(location=6) in float rotation;

layout(location=0) uniform vec3 cameraRightVector;
layout(location=1) uniform vec3 cameraUpVector;
layout(location=2) uniform mat4 VP;
layout(location=3) uniform float currentFrame;
layout(location=4) uniform vec2 tileSize;

flat out vec4 fragParticleColor;
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

    fragParticleColor = particleColor;

    mat3 zRotation = mat3(
        cos(rotation), -sin(rotation), 0.f, 
        sin(rotation), cos(rotation), 0.f, 
        0.f, 0.f, 1.f
    );

    vec3 rightRotated = normalize(zRotation * cameraRightVector);
    vec3 upRotated = normalize(zRotation * cameraUpVector);

    gl_Position = VP * vec4(billboardCenter + rightRotated * vertexPosition.x * billboardSize.x
    + upRotated * vertexPosition.y * billboardSize.y, 1.f);
}