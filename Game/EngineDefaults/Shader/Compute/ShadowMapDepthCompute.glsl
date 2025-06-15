#version 460

uniform sampler2D inTexture;
uniform writeonly image2D outImage;
uniform ivec2 inSize;

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
shared vec2 values[gl_WorkGroupSize.x * gl_WorkGroupSize.y];

void main()
{
    if(gl_GlobalInvocationID.x < inSize.x && gl_GlobalInvocationID.y < inSize.y) // work item inside work domain
    {
        ivec2 inCoord = ivec2(gl_GlobalInvocationID.xy);
        float depth = texelFetch(inTexture, inCoord, 0).r;
        values[gl_LocalInvocationIndex].x = depth;
        values[gl_LocalInvocationIndex].y = (depth < 1.0) ? depth : 0.0;
    }
    else {
        values[gl_LocalInvocationIndex] = vec2(1.0, 0.0);
    }

    // Synchronization
    memoryBarrierShared();
    barrier();


    if(gl_LocalInvocationIndex == 0)
    {
        float minValue = 1.0;
        float maxValue = 0.0;

        for(int i=0; i< int(gl_WorkGroupSize.x * gl_WorkGroupSize.y); ++i) {
            minValue = min(minValue, values[i].x);
            maxValue = max(maxValue, values[i].y);
        }

        imageStore(outImage, ivec2(gl_WorkGroupID.xy), vec4(minValue, maxValue, 0.0, 0.0));
    }
}