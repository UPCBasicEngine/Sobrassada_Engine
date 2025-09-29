#version 460

out vec2 uv0;

void main() {
    vec2 pos;
    
    if (gl_VertexID == 0)
        pos = vec2(-1.0, -1.0);
    else if (gl_VertexID == 1)
        pos = vec2(3.0, -1.0);
    else
        pos = vec2(-1.0, 3.0);

    uv0 = (pos + 1.0) * 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}