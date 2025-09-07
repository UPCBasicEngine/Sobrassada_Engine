#version 460

#extension GL_ARB_bindless_texture : require

layout(location = 4) uniform uvec2 myTexture;

in vec2 uv;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(sampler2D(myTexture), uv);
    // fragColor = vec4(0, 0, 1.0f, 1.0f);
    fragColor = texColor;
}