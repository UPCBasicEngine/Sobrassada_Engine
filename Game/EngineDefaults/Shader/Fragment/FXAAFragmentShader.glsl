#version 460

in vec2 uv0;

layout(binding=0) uniform sampler2D lastDrawTexture;

layout(location=0) uniform bool showBorders;
layout(location=1) uniform float globalThreshold;
layout(location=2) uniform float localThreshold;
layout(location=3) uniform bool enableFXAA;

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

vec2 ComputeBlendDirection(float middle, float neighbour, float oppositeNeighbour)
{
    vec2 pixelStep;
    float pGradient = abs(neighbour - middle);
    float nGradient = abs(oppositeNeighbour - middle);
    if (pGradient >= nGradient)
    {
        pixelStep = vec2(0.0f, 1.0f / textureSize(lastDrawTexture, 0).y);
    }
    else
    {
        pixelStep = vec2(0.0f, -1.0f / textureSize(lastDrawTexture, 0).y);
    }
    return pixelStep;
}

vec3 ApplyFXAA(float middle, float north, float east, float south, float west, float contrast) 
{
    // Pixel blend factor
    float northEast = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, 1)));
    float southEast = luma(textureOffset(lastDrawTexture, uv0, ivec2(1, -1)));
    float southWest = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, -1)));
    float northWest = luma(textureOffset(lastDrawTexture, uv0, ivec2(-1, 1)));

    float average = 2.0f * (north + east + south + west) + northEast + southEast + southWest + northWest;
    average /= 12.0f;

    float pixelBlendFactor = abs(middle - average) / contrast;
    pixelBlendFactor = smoothstep(0.0f, 1.0f, clamp(pixelBlendFactor, 0.0f, 1.0f));
    pixelBlendFactor *= pixelBlendFactor;


    // Edge blend direction
    float verticalEdge = abs((0.25 * northWest) + (-0.5 * north) + (0.25 * northEast)) +        
                        abs((0.50 * west ) + (-1.0 * middle) + (0.50 * east )) + 
                        abs((0.25 * southWest) + (-0.5 * south) + (0.25 * southEast)); 
                        
    float horizontalEdge = abs((0.25 * northWest) + (-0.5 * west) + (0.25 * southWest)) + 
                        abs((0.50 * north ) + (-1.0 * middle) + (0.50 * south)) + 
                        abs((0.25 * northEast) + (-0.5 * east) + (0.25 * southEast));
    
    bool isHorizontal = horizontalEdge >= verticalEdge; 

    float neighbour = isHorizontal ? north : east;
    float oppositeNeighbour = isHorizontal ? east : west;

    vec2 pixelStep = ComputeBlendDirection(middle, neighbour, oppositeNeighbour);   
    return texture(lastDrawTexture, uv0 + pixelStep * pixelBlendFactor).rgb;
}

void main()
{
    if (!enableFXAA)  // If disabkle, just pass-through. Needed for drawing to screen framebuffer anyway
    {
        fragColor = texture(lastDrawTexture, uv0);
        return;
    }

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
        if (showBorders) 
        {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else 
        {
            fragColor = vec4(ApplyFXAA(middle, north, east, south, west, contrast), 1.0f);
        }
    } 
}