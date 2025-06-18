#version 460

uniform sampler2D inTexture;
uniform writeonly image2D outImage;
uniform ivec2 inSize;

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
shared float minvalues[gl_WorkGroupSize.x * gl_WorkGroupSize.y];
shared float maxvalues[gl_WorkGroupSize.x * gl_WorkGroupSize.y];

void main()
{
    if(gl_GlobalInvocationID.x < inSize.x && gl_GlobalInvocationID.y < inSize.y) // work item inside work domain
    {
        ivec2 inCoord = ivec2(gl_GlobalInvocationID.xy);
        vec2 depth = texelFetch(inTexture, inCoord, 0).rg;
        minvalues[gl_LocalInvocationIndex] = depth.r;
        maxvalues[gl_LocalInvocationIndex] = depth.g;
    }
    else {
        minvalues[gl_LocalInvocationIndex] = 1.0;
        maxvalues[gl_LocalInvocationIndex] = 0.0;
    }

    // Synchronization
    memoryBarrierShared();
    barrier();


    if(gl_LocalInvocationIndex == 0)
    {
        float minValue = 1.0;
        float maxValue = 0.0;

        for(int i = 0; i< int(gl_WorkGroupSize.x * gl_WorkGroupSize.y); ++i) {
            minValue = min(minValue, minvalues[i]);
            if (maxvalues[i] < 1.0) {
                maxValue = max(maxValue, maxvalues[i]);
            }
        }

        imageStore(outImage, ivec2(gl_WorkGroupID.xy), vec4(minValue, maxValue, 0.0, 0.0));
    }
}