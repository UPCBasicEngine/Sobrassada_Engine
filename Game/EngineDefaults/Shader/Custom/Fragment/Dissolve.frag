#version 460

#extension GL_ARB_bindless_texture : require

layout(binding=0) uniform sampler2D noiseTexture;

layout(location=3) uniform float dissolveAmmount;

in vec2 uv;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(sampler2D(noiseTexture), uv);

    float isVisible = (texColor.r * 0.99) - dissolveAmmount;
    
    if(isVisible < 0) discard;
    
    fragColor = texColor;
}