#version 460

in vec3 pos;
in vec2 uv0;
in vec3 normal;
out vec4 outColor;

layout(binding=0) uniform sampler2D diffuseTexture;
layout(binding=1) uniform sampler2D specularTexture;

uniform vec3 lightColor;
uniform vec3 lightDir;
uniform vec3 cameraPos;

/**
 * Lights Data
**/ 
layout(std140, binding = 4) uniform Ambient
{
	vec4 ambient_color;		// rbg = color & alpha = intensity
};



layout(std140, binding = 1) uniform Material
{
    vec4 diffColor;
    vec4 specColor;
    float shininess;   
    bool shininessInAlpha;  
};

void main()
{
    vec3 texColor = pow(texture(diffuseTexture, uv0).rgb, vec3(2.2f));
    vec4 specTexColor = texture(specularTexture, uv0);
    float alpha = specTexColor.a;

    //TEMP: Texture colors to see lights effect
    texColor = vec3(0.5f, 0.5f, 0.5f);
    specTexColor = vec4(0.5f, 0.5f, 0.5f, 0.5f);
    alpha = specTexColor.a;

    // Ambient light
    vec3 ambient = ambient_color.rgb * ambient_color.a;
    vec3 hdr = ambient * texColor;

    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);

    float NL = dot(N, -L);
    if (NL > 0)
    {
		float shininessValue;
		if(shininessInAlpha) shininessValue = exp2(alpha * 7 + 1);
		else shininessValue = shininess;

        float normalization = (shininessValue + 2.0) / (2.0 * 3.1415926535);
        vec3 V = normalize(cameraPos - pos);
        vec3 R = reflect(L, N);
        float VR = pow(max(dot(V, R), 0.0f), shininessValue);
        
        vec3 RF0 = specTexColor.rgb;
        float cosTheta = max(dot(N, V), 0.0);
        vec3 fresnel = RF0 + (1 - RF0) * pow(1 - cosTheta, 5);

        vec3 diffuse = (1.0 - RF0) / 3.1415926535 * diffColor.rgb * texColor * lightColor * NL;
        vec3 specular = normalization * specColor.rgb * specTexColor.rgb * VR * lightColor * fresnel;
        hdr = ambient + diffuse + specular;
    }

    hdr = pow(hdr, vec3(1/2.2));
    outColor = vec4(hdr, alpha);
}