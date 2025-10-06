#version 460

layout(binding=0) uniform sampler2D myTexture;

in vec2 uv;
flat in vec4 fragParticleColor;
flat in float fragColorIntensity;

out vec4 fragColor;

void main()
{
    const vec4 color0 = texture2D(myTexture, uv);
    fragColor = vec4(fragParticleColor.rgb * color0.rgb * fragColorIntensity, min(color0.a, fragParticleColor.a));
}