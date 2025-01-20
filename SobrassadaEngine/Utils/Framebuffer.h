#pragma once

#include <memory>

class Framebuffer
{
public:
	Framebuffer(int witdh, int height, bool useRbo);
	~Framebuffer();

	void Bind();
	void Unbind();
	void Resize(int width, int height);
	void CheckResize();

	const unsigned int GetFramebufferID() { return fbo; };
	const unsigned int GetTextureID() { return framebufferTexture; }
	const unsigned int GetTextureWidth() { return textureWidth; }
	const unsigned int GetTextureHeight() { return textureHeight; }

private:
	bool Initialize();
	void CleanUp();

	unsigned int fbo, rbo;
	unsigned int framebufferTexture;
	int textureWidth, textureHeight;
	bool shouldResize, useRbo;
};

