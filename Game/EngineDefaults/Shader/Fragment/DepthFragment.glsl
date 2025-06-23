#version 460

uniform sampler2D u_Texture;
in vec2 uv0;
out vec4 FragColor;

void main()
{
    float depth = texture(u_Texture, uv0).r;
    FragColor = vec4(vec3(depth), 1.0);
}