#version 460

uniform layout(binding = 0) writeonly image2D transmitanceTexture;
uniform layout(binding = 1) sampler2D depthTexture;

uniform layout(location = 0) float width;
uniform layout(location = 1) float height;
uniform layout(location = 2) mat4 inverseProjectionMatrix;
uniform layout(location = 3) mat4 inverseViewMatrix;

uniform layout(location = 4) float zNear;
uniform layout(location = 5) float zFar;
uniform layout(location = 6) vec3 cameraPosition;

// JUST FOR TESTING REMOVE AT SOME POINT
uniform layout(location=7) float blendFactor;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

vec3 GetWorldPosition(float depth, vec2 uv)
{
    vec3 position = vec3(uv,depth);

    vec4 clipPosition = vec4(position * 2.0 - 1.0, 1);

    // Linearize depth, zFar is very big so all values are very close to 1
    clipPosition.z = 2.0 * zNear * zFar / (zFar + zNear - clipPosition.z * (zFar - zNear));

    vec4 viewPosition = inverseProjectionMatrix * clipPosition;

    viewPosition /= viewPosition.w;

    vec4 worldPosition = inverseViewMatrix * viewPosition;

    return worldPosition.xyz + cameraPosition;
    // return worldPosition.xyz;
}

void main()
{
    if(gl_GlobalInvocationID.x < width && gl_GlobalInvocationID.y < height)
    {
        vec2 uvCoords = vec2(gl_GlobalInvocationID.x / width, gl_GlobalInvocationID.y / height);
        float depth = texture(depthTexture, uvCoords).r;

        imageStore(transmitanceTexture, ivec2(gl_GlobalInvocationID.xy), vec4(GetWorldPosition(depth, uvCoords), blendFactor));
    }
}