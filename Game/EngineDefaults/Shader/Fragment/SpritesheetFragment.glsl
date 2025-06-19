#version 460

layout(binding=0) uniform sampler2D myTexture;

in vec2 uv;
in vec2 uvNext;
flat in float blendFactor;

out vec4 fragColor;

void main()
{
    const vec4 color0 = texture2D(myTexture, vec2(uv.x, 1 - uv.y));
    const vec4 color1 = texture2D(myTexture, vec2(uvNext.x, 1 - uvNext.y));

    fragColor = mix(color0, color1, blendFactor);
}