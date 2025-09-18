#include "Framebuffer.h"

#include "Application.h"

#include "glew.h"

Framebuffer::Framebuffer(int witdh, int height)
    : fbo(0), framebufferTexture(0), textureWidth(witdh), textureHeight(height), shouldResize(false)
{

    CreateTexture(witdh, height);
    if (!framebufferTexture || !Initialize()) GLOG("Error creating framebuffer!")
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &fbo);

    glDeleteTextures(1, &framebufferTexture);
    glDeleteTextures(1, &framebufferDepthTexture);
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

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, framebufferDepthTexture, 0);

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

    if (framebufferDepthTexture == 0) glGenTextures(1, &framebufferDepthTexture);

    glBindTexture(GL_TEXTURE_2D, framebufferDepthTexture);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
        NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
}
