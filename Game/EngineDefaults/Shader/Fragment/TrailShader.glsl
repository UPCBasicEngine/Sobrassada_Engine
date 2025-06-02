#version 460

uniform bool useTexture;
uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;

in vec4 vColor;
in vec2 vUV;

void main()
{
    if (useTexture){
        vec4 diffuse = texture(uTexture, vUV);
        if(diffuse.a < 0.1) discard;
        FragColor = vec4(diffuse.rgb * vColor.rgb, diffuse.a);
    }
    else
        FragColor = vColor;
}