#version 460

layout(binding = 0) uniform sampler2D gDepth;

layout(location = 2) uniform float densityConstant;
layout(location = 3) uniform float heightFalloff;

layout(location = 4) uniform vec3 cameraPos;
layout(location = 5) uniform mat4 cameraMatrix;
layout(location = 6) uniform mat4 projection;
layout(location = 7) uniform float maxFog;
layout(location = 8) uniform vec3 fogColor;
layout(location = 9) uniform float fogStartHeight;

in vec2 uv0;
out vec4 fragColor;

float ApplyFog(float distToPoint, vec3 camToPoint)
{
    float heightOffset = cameraPos.y - fogStartHeight;
    return densityConstant * exp(-heightOffset * heightFalloff) * (1.0f - exp(-distToPoint * camToPoint.y * heightFalloff)) / (camToPoint.y * heightFalloff);
}

vec3 GetWorldPosition(float depth)
{
    float a = projection[3][2];
    float b = projection[2][2];

    float zView = - a / ((depth * 2.0 - 1.0) + b);
    
    a = projection[0][0];
    b = projection[1][1];

    float xView = (-zView / a) * (uv0.x * 2.0 - 1.0);
    float yView = (-zView / b) * (uv0.y * 2.0 - 1.0);

    vec4 worldPosition = cameraMatrix * vec4(xView, yView, zView, 1.0);

    return worldPosition.xyz;
}

void main()
{
    float depth = texture(gDepth, uv0).r;

    vec3 worldPos = GetWorldPosition(depth);
    vec3 rayDir = worldPos - cameraPos;
    float distToPoint = length(rayDir);

    float fogAmount = min(ApplyFog(distToPoint, rayDir / distToPoint), maxFog);

    fragColor = vec4(fogColor, fogAmount);
}