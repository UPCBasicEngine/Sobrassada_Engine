#version 460

layout(binding=0) uniform sampler2D myTexture;

in vec2 uv;
in vec2 uvNext;
flat in float blendFactor;
flat in vec4 fragParticleColor;

out vec4 fragColor;

void main()
{
    vec4 color0 = texture2D(myTexture, vec2(uv.x, 1 - uv.y));
    vec4 color1 = texture2D(myTexture, vec2(uvNext.x, 1 - uvNext.y));
    vec4 textureColor = mix(color0, color1, blendFactor);
    // fragColor = mix(textureColor, fragParticleColor, textureColor.w);
    if(textureColor.w > 0.f ) fragColor = fragParticleColor;
}