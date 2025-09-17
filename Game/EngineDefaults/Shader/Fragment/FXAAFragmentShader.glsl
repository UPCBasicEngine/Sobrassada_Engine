#version 460

in vec2 uv0;

layout(binding=0) uniform sampler2D lastDrawTexture;

layout(location=0) uniform bool showBorders;
layout(location=1) uniform float globalThreshold;
layout(location=2) uniform float localThreshold;

out vec4 fragColor;

float luma(const vec4 color)
{
    return color.g;
}

float getMax(float values[5])
{
    float maxValue = 0;
    for (int i = 0; i < 5; ++i) {
        if (maxValue < values[i]) maxValue = values[i];
    }

    return maxValue;
}

float getMin(float values[5])
{
    float minValue = 1;
    for (int i = 0; i < 5; ++i) {
        if (minValue > values[i]) minValue = values[i];
    }

    return minValue;
}

void main()
{
    float middle = luma(texture(lastDrawTexture, uv0));
    float north = luma(textureOffset(lastDrawTexture, uv0, ivec2(0, 1)));
    float east = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, 0)));
    float south = luma(textureOffset(lastDrawTexture, uv0, ivec2(0, -1)));
    float west = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, 0)));
    
    float luminance[5] = float[5](middle, north, east, south, west);
    float maxValue = getMax(luminance);
    float minValue = getMin(luminance);
    float contrast = maxValue - minValue;

    if(contrast < max(localThreshold * maxValue, globalThreshold))
    {
        fragColor = texture(lastDrawTexture, uv0);
    }
    else
    {
        if (showBorders) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
             fragColor = texture(lastDrawTexture, uv0);
        }
        //fragColor = applyFXAA(luminance);
    }


    
}