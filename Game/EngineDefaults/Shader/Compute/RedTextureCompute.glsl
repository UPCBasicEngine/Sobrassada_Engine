#version 460

uniform layout(binding=5, rgba8) writeonly image2D myTexture;

uniform layout(location=0) int width;
uniform layout(location=1) int height;
uniform layout(location=2) float blendFactor;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    if(gl_GlobalInvocationID.x < width && gl_GlobalInvocationID.y < height) 
    {
        imageStore(myTexture, ivec2(gl_GlobalInvocationID.xy), vec4(1.0, 0.0, 0.0, blendFactor));
    }
}