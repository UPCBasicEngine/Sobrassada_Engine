#include "Framebuffer.h"

#include "Application.h"
#include "TextureModuleTest.h"

#include "glew.h"

Framebuffer::Framebuffer(int witdh, int height, bool useRbo)
    : fbo(0), rbo(0), framebufferTexture(0), textureWidth(witdh), textureHeight(height), shouldResize(false),
      useRbo(useRbo)
{

    CreateTexture(witdh, height);
    if (!framebufferTexture || !Initialize()) GLOG("Error creating framebuffer!")
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &fbo);
    if (useRbo) glDeleteRenderbuffers(1, &rbo);

    glDeleteTextures(1, &framebufferTexture);
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height)
{
    textureWidth  = width;
    textureHeight = height;
    shouldResize  = true;
}

void Framebuffer::CheckResize()
{
    if (!shouldResize) return;

    CreateTexture(textureWidth, textureHeight);

    Initialize();

    shouldResize = false;
}

bool Framebuffer::Initialize()
{
    if (fbo == 0) glCreateFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    if (useRbo)
    {
        if (rbo == 0) glCreateRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);

        // IF SHADOWS GL_DEPTH_COMPONENT32F
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, textureWidth, textureHeight);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        GLOG("ERROR::FRAMEBUFFER::Framebuffer is not complete!\n");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void Framebuffer::CreateTexture(int width, int height)
{
    if (framebufferTexture == 0) glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}
