#version 460

layout(binding=0) uniform sampler2D myTexture;

in vec2 uv;
// in vec2 uvNext;
// flat in float blendFactor;
flat in vec4 fragParticleColor;
flat in float fragColorIntensity;

out vec4 fragColor;

void main()
{
    // const vec4 color0 = texture2D(myTexture, vec2(uv.x, uv.y));
    // const vec4 color1 = texture2D(myTexture, vec2(uvNext.x, uvNext.y));
    // const vec4 textureColor = mix(color0, color1, blendFactor);
    // fragColor = vec4(fragParticleColor.rgb * textureColor.rgb * fragColorIntensity, min(textureColor.a, fragParticleColor.a));

    const vec4 color0 = texture2D(myTexture, uv);
    fragColor = vec4(fragParticleColor.rgb * color0.rgb * fragColorIntensity, min(color0.a, fragParticleColor.a));
}