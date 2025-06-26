#version 460
uniform sampler2D u_Texture;
in vec2 uv0;
out vec4 color;

void main() {
    const float d = texture(u_Texture, uv0).r;
    color = vec4(vec3(d), 1.0); // Visualize as grayscale
}