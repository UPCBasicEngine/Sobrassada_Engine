#include "SSAO.h"

#include "Algorithm/Random/LCG.h"
#include "GBuffer.h"
#include "Globals.h"
#include "glew.h"

#include <random>

SSAO::SSAO(int width, int height)
{
    screenWidth  = width;
    screenHeight = height;
    shouldResize = true;
    Init();
}

SSAO::~SSAO()
{
    glDeleteFramebuffers(1, &ssaoFrameBufferObject);

    glDeleteTextures(1, &ssaoTexture);
}

void SSAO::Init()
{
    if (ssaoFrameBufferObject == 0) glGenFramebuffers(1, &ssaoFrameBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFrameBufferObject);

    // ssao
    if (ssaoTexture == 0) glGenTextures(1, &ssaoTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) printf("SSAO FBO not complete!\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    kernels.clear();

    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    std::default_random_engine generator;

    for (int i = 0; i < SSAO_KERNEL_SIZE_MID; ++i)
    {
        //float3 kernel = float3::RandomDir(*rng);
        float3 kernel;
        kernel.x = distribution(generator) * 2.f - 1.f;
        kernel.y = distribution(generator) * 2.f - 1.f;
        kernel.z = distribution(generator);

        //if (kernel.z < 0.0f) // flip points in lower hemishpere
        //    kernel.z *= -1,.0f;
        kernel.Normalize();

        float scale  = float(i) / float(SSAO_KERNEL_SIZE_MID);
        scale        = 0.1f + 0.9f * (scale * scale); // Near-origin bias
        kernel      *= scale;

        kernels.push_back(kernel);
    }

    
    noise.reserve(SSAO_KERNEL_SIZE_LOW);

    for (int i = 0; i < SSAO_KERNEL_SIZE_LOW; i++)
    {
        float3 n(rng->Float(-1.0f, 1.0f), rng->Float(-1.0f, 1.0f), 0.0f);
        n.Normalize();
        noise.push_back(n);
    }

    
    if(noiseTexture == 0) glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    
}

void SSAO::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFrameBufferObject);
}

void SSAO::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::Resize(int width, int height)
{
    screenWidth  = width;
    screenHeight = height;
    shouldResize = true;
}

void SSAO::CheckResize()
{
    if (!shouldResize) return;

    Init();

    shouldResize = false;
}
