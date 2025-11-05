#version 460

layout(binding = 0) uniform sampler2D gPositions;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gDepth;
layout(binding = 3) uniform sampler2D noiseTexture;
layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

uniform vec2 screenSize;
in vec2 uv0;

uniform float bias;
uniform float range; 
 
const int KERNEL_SIZE = 32;
uniform vec3 kernel_samples[KERNEL_SIZE];
uniform vec3 random_tangents[KERNEL_SIZE];
out vec4 result;

mat3 createTangentSpace(const vec3 normal, const vec3 up)
{
   const vec3 tangent   = normalize(up-normal * dot(normal, up));
   const vec3 bitangent = normalize(cross(tangent, normal)); 
   return mat3(bitangent, tangent, normal);
}

vec3 getRandomTangent() {
   const vec2 noiseScale = screenSize / 4.0;
   return normalize(texture(noiseTexture, uv0 * noiseScale).xyz);
}

void main()
{
    const vec3 position     = (viewMatrix * vec4(texture(gPositions, uv0).xyz, 1.0)).xyz;
    const vec3 normal       = mat3(viewMatrix) * normalize(texture(gNormals, uv0).xyz);
    const mat3 tangentSpace = createTangentSpace(normal, getRandomTangent());
    int occlusion     = 0;

    for(int i=0; i< KERNEL_SIZE; ++i) 
    {
        vec3 samplePos = position + tangentSpace * kernel_samples[i];

        const vec4 clipSpace = projMatrix * vec4(samplePos, 1.0);
        const vec2 sampleUV = (clipSpace.xy/clipSpace.w) * 0.5 + 0.5;

        if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;
        float sampleDepth = (viewMatrix * texture(gPositions, sampleUV)).z;

        if(sampleDepth + bias > samplePos.z && abs(sampleDepth - position.z) < range)
        {
            ++occlusion;
        }
    }

    const float ao = 1.0 - float(occlusion) / float(KERNEL_SIZE);
    result = vec4(vec3(ao), 1.0);
}

