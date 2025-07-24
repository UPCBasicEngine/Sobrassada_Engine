#include "GBuffer.h"

#include "Globals.h"

#include "glew.h"

GBuffer::GBuffer(int width, int height)
{
    colorAttachments[0] = GL_COLOR_ATTACHMENT0;
    colorAttachments[1] = GL_COLOR_ATTACHMENT1;
    colorAttachments[2] = GL_COLOR_ATTACHMENT2;
    colorAttachments[3] = GL_COLOR_ATTACHMENT3;

    screenWidth         = width;
    screenHeight        = height;

    InitializeGBuffer();
}

GBuffer::~GBuffer()
{
    glDeleteFramebuffers(1, &gBufferObject);

    glDeleteTextures(1, &positionTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
    glDeleteTextures(1, &depthTexture);
}

void GBuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferObject);
}

void GBuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::Resize(int width, int height)
{
    screenWidth  = width;
    screenHeight = height;
    shouldResize = true;
}

void GBuffer::CheckResize()
{
    if (!shouldResize) return;

    InitializeGBuffer();

    shouldResize = false;
}

void GBuffer::InitializeGBuffer()
{
    if (gBufferObject == 0) glGenFramebuffers(1, &gBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferObject);

    // diffuse
    if (diffuseTexture == 0) glGenTextures(1, &diffuseTexture);
    glBindTexture(GL_TEXTURE_2D, diffuseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, diffuseTexture, 0);

    // specular
    if (specularTexture == 0) glGenTextures(1, &specularTexture);
    glBindTexture(GL_TEXTURE_2D, specularTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, specularTexture, 0);

    // position
    if (positionTexture == 0) glGenTextures(1, &positionTexture);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, positionTexture, 0);

    // normal
    if (normalTexture == 0) glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, normalTexture, 0);

    glDrawBuffers(4, colorAttachments);

    if (depthTexture == 0) glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screenWidth, screenHeight, 0, GL_DEPTH_STENCIL,
        GL_UNSIGNED_INT_24_8,
        NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        GLOG("ERROR::GBUFFER::GBUFFER is not complete!\n");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}