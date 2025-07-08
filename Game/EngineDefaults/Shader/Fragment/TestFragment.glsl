#version 460

layout(binding=0) uniform sampler2D myTexture;

in vec2 uv;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture2D(myTexture, vec2(uv.x, 1 - uv.y));
    // fragColor = vec4(0, 0, 1.0f, 1.0f);
    fragColor = texColor;
}