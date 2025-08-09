#version 460

layout(location=0) in vec3 vertex_position;
layout(location=1) in vec4 vertex_tangent;
layout(location=2) in vec3 vertex_normal;
layout(location=3) in vec2 vertex_uv0;
layout(location=4) in ivec4 vertex_joint;
layout(location=5) in vec4 vertex_weights;

layout(location=4) uniform bool hasBones;
uniform mat4 viewLight;
uniform mat4 projLight;

// xyzw: quaternion for the wind direction
uniform vec4 windDirection;
// x: currentTime (set to 0 disables the wind), y: wind speed, z: gust frequency, y: gust speed
uniform vec4 windParameters;

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

out vec3 pos;
out vec3 normal;
out vec2 uv0;
out vec4 tangent;
out vec3 fragViewPos;
flat out int instance_index;

void main()
{
    instance_index = gl_BaseInstance;
    mat4 model = models[instance_index];

    //Camera position in World Space
    fragViewPos = vec3(inverse(viewMatrix)[3]);
    uv0 = vertex_uv0;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    normal = normalMatrix * vertex_normal;
    tangent = vec4(normalMatrix * vertex_tangent.xyz, vertex_tangent.w);

    // Indexing with a float is crashing
    if (hasBones) 
    {
        uint boneIndex = bonesIndex[instance_index];
        mat4 skin    = palettes[boneIndex + vertex_joint[0]] * vertex_weights[0] + palettes[boneIndex + vertex_joint[1]] * vertex_weights[1] +             
                       palettes[boneIndex + vertex_joint[2]] * vertex_weights[2] + palettes[boneIndex + vertex_joint[3]] * vertex_weights[3];
        pos          = (skin * vec4(vertex_position, 1.0)).xyz;

        mat3 skinRot = mat3(skin); // Skin matrix with rotation only
        normal       = skinRot * vertex_normal;        
        tangent      = vec4(skinRot * tangent.xyz, tangent.w); 
    } 
    else 
    {
        pos = vec3(model * vec4(vertex_position, 1.0));

// windDirection: xyzw: quaternion for the wind direction
// windParameters: x: currentTime (set to 0 disables the wind), y: wind speed, z: gust frequency, y: gust speed

        if (bool(windParameters.x))
        {
            float gustStrength = max(0, sin((windParameters.x * 0.001) / windParameters.z));

            float combinedWindSpeed = windParameters.y + (gustStrength * windParameters.w);
            float scaledTime = windParameters.x * 0.001 * (log(windParameters.y * 2) + 1);
            float scaledWindSpeed = combinedWindSpeed * 0.2;

            float offsetX = sin(pos.x + scaledTime * 1.25 + uv0.y) * (1.0 - uv0.y) * 0.2 * scaledWindSpeed;
            float offsetY = cos(pos.y + scaledTime * 0.2 + uv0.y) * (1.0 - uv0.y) * 0.15 * scaledWindSpeed;
            float offsetZ = cos(pos.z + scaledTime * 0.2 + uv0.y) * (1.0 - uv0.y) * 0.15 * scaledWindSpeed;

            vec3 offset = vec3(offsetX, offsetY, offsetZ);

            vec3 temp = cross(windDirection.xyz, offset) + windDirection.w * offset;
            vec3 rotated = offset + 2.0*cross(windDirection.xyz, temp);

            pos = pos + rotated;
        }
    }

    gl_Position = projMatrix * viewMatrix * vec4(pos, 1.0f); 
}