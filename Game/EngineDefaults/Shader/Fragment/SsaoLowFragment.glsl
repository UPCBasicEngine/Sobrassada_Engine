#version 460

layout(binding = 0) uniform sampler2D gPositions;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gDepth;

layout(std140, row_major, binding = 0) uniform CameraMatrices
{
    mat4 projMatrix;
    mat4 viewMatrix;
};

uniform vec2 screenSize;
in vec2 uv0;

const int KERNEL_SIZE = 32;
uniform vec3 kernel_samples[KERNEL_SIZE];
uniform vec3 random_tangents[KERNEL_SIZE];

out vec4 result;

mat3 createTangentSpace(const vec3 normal, const vec3 up)
{
   vec3 tangent   = normalize(up-normal*dot(normal, up));
   vec3 bitangent = normalize(cross(tangent, normal)); 
   return mat3(bitangent, tangent, normal);
}

vec3 getRandomTangent()
{
    vec2 screenPos = uv0 * screenSize;
    int index = int(mod(screenPos.x + screenPos.y, float(KERNEL_SIZE)));
    return random_tangents[index];
}

float getSceneDepthAtSamplePos(in vec3 samplePos)
{
    vec4 clipSpace = projMatrix * vec4(samplePos, 1.0);
    vec2 sampleUV = (clipSpace.xy/clipSpace.w)*0.5+0.5;

    //if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
    //return 1e6;

    return (viewMatrix*texture(gPositions, sampleUV)).z;
}

void main()
 {
    vec3 position     = (viewMatrix*vec4(texture(gPositions, uv0).xyz, 1.0)).xyz;
    vec3 normal       = mat3(viewMatrix)*normalize(texture(gNormals, uv0).xyz);
    mat3 tangentSpace = createTangentSpace(normal, getRandomTangent());
    int occlusion     = 0;

    for(int i=0; i< KERNEL_SIZE; ++i) 
    {
        vec3 samplePos = position+tangentSpace*kernel_samples[i];
        float sampleDepth = getSceneDepthAtSamplePos(samplePos);
        if(sampleDepth > samplePos.z)
	{
	 ++occlusion;
	}
    }

    //if(getSceneDepthAtSamplePos(samplePos) > samplePos.z) ++occlusion;
    //if ((sampleDepth< -samplePos.z) && abs(sampleDepth - position.z))
    
    float ao = 1.0 - float(occlusion) / float(KERNEL_SIZE);
    result = vec4(vec3(ao), 1.0);

 }

