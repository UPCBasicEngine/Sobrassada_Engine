#version 460

#extension GL_ARB_bindless_texture : require

in vec2 uv0;
out vec4 outColor;

layout(location = 3) uniform vec3 inputColor;
layout(location = 4) uniform uvec2 fontTexture;
layout(location = 5) uniform uint widgetType;

void main()
{
    vec4 sampled = texture(sampler2D(fontTexture), uv0);

    if (sampled.a < 0.05) discard;

    switch(widgetType) {
        // LABEL
        case 0:    
            outColor = vec4(inputColor.r, inputColor.g, inputColor.b, sampled.r);
            break;
        
        // IMAGE
        case 1:
            outColor = vec4(sampled.r * inputColor.r, sampled.g * inputColor.g, sampled.b * inputColor.b, sampled.a);
            break;

        // FALLBACK
        default:
            outColor = vec4(inputColor.r, inputColor.g, inputColor.b, 1);
            break;
    }
    
}
