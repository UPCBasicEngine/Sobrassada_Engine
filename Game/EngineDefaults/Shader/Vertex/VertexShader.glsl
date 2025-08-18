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
uniform vec4 deltaWindDirection;
// x: currentTime (set to 0 disables the wind), y: wind speed, z: gust frequency, y: gust speed
uniform vec4 windParameters;
// x: v0 (no movement border), y: v1 (full movement border), z: use central pivot, w: use gravity
uniform vec4 windUVParameters;
// x: axis affect, y: axis affect, z: axis affect, w: windResistance
uniform vec4 windCustomStrength;

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
        // windDirection: xyzw: quaternion for the wind direction
    // windParameters: x: currentTime (set to 0 disables the wind), y: wind speed, z: gust frequency, y: gust speed
    // windUVParameters: x: v0 (no movement border), y: v1 (full movement border), z: use central pivot, w: use gravity
    // windCustomStrength: x: axis affect, y: axis affect, z: axis affect, w: windResistance

        pos = vertex_position;

        if (bool(windParameters.x))
        {
            float vCoordLerp = max(min(((1-uv0.y) - windUVParameters.x) / (windUVParameters.y - windUVParameters.x), 1), 0);

            if (bool(windUVParameters.z))
            {
                vec3 tempLocal = cross(deltaWindDirection.xyz, pos) + deltaWindDirection.w * pos;
                vec3 rotatedAroundPivot = pos + 2.0*cross(deltaWindDirection.xyz, tempLocal);

                pos = rotatedAroundPivot;
            }

            // Gravity pulling parts further from origin down
            if (bool(windUVParameters.w)) {
                float distanceToPivotSq = pos.x * pos.x + pos.z * pos.z;
                pos.y -= distanceToPivotSq * 0.1;
            }

            float gustStrength = max(0, sin((windParameters.x * 0.001) / windParameters.z));

            float combinedWindSpeed = windParameters.y + (gustStrength * windParameters.w);
            float scaledTime = windParameters.x * 0.001 * (log(windParameters.y * 2) + 1);
            float scaledWindSpeed = combinedWindSpeed * (1 - windCustomStrength.w);

            float basicOffset = sin(pos.x + scaledTime + 1.0 - vCoordLerp) * vCoordLerp * scaledWindSpeed;

            vec3 offset = vec3(basicOffset * windCustomStrength.x, basicOffset * windCustomStrength.y, basicOffset * windCustomStrength.z);

            vec3 temp = cross(deltaWindDirection.xyz, offset) + deltaWindDirection.w * offset;
            vec3 rotated = offset + 2.0*cross(deltaWindDirection.xyz, temp);

            pos = pos + rotated;
        }

        /*if (bool(windParameters.x) && bool(windUVParameters.z)) {
            // If wind is activated with wind rotations around the pivot ignore the model rotation
            mat4 scalingMatrix = transpose(model) * model;

            mat4 modelWithoutRotation;
            modelWithoutRotation[3][0] = model[3][0];
            modelWithoutRotation[3][1] = model[3][1];
            modelWithoutRotation[3][2] = model[3][2];
            modelWithoutRotation[0][0] = sqrt(scalingMatrix[0][0]);
            modelWithoutRotation[1][1] = sqrt(scalingMatrix[1][1]);
            modelWithoutRotation[2][2] = sqrt(scalingMatrix[2][2]);
            model = modelWithoutRotation;
        }*/

        pos = vec3(model * vec4(pos, 1.0));
    }

    gl_Position = projMatrix * viewMatrix * vec4(pos, 1.0f); 
}