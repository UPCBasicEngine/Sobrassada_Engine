#version 460
out vec4 color;
in vec2 uv0;

layout(binding=0) uniform sampler2D myTexture;

void main()
{
	// color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	color = texture2D(myTexture, uv0);
}