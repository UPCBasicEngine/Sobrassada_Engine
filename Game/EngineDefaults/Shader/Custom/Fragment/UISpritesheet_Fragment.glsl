#version 460

#extension GL_ARB_bindless_texture : require

layout(location = 4) uniform uvec2 myTexture;

in vec2 uv0;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(sampler2D(myTexture), uv0);
    
    if (texColor.a < 0.1f) {
        discard;
    }

    fragColor = texColor;
}