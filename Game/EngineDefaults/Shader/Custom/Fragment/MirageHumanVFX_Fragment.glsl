#version 460

layout(location = 3) uniform vec3 color;

in vec2 uv;
in vec3 normal;

out vec4 fragColor;

void main()
{

    float alpha = pow(1 - abs(dot(normalize(normal), vec3(0, 0, 1))), 1.640);

    fragColor = vec4(color * vec3(alpha), alpha);
}