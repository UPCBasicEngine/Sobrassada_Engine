uniform sampler2D u_Texture;
in vec2 TexCoords;
out vec4 FragColor;
uniform float nearPlane;
uniform float farPlane;

float LinearizeDepth(float depth)
{
    return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
}

void main()
{
    float depth = texture(u_Texture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth);
    FragColor = vec4(vec3(linearDepth), 1.0);
}