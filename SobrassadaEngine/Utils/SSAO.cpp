#include "SSAO.h"

#include "Algorithm/Random/LCG.h"
#include "GBuffer.h"
#include "Globals.h"
#include "glew.h"

SSAO::SSAO(int width, int height)
{
    screenWidth  = width;
    screenHeight = height;

    Init();
}

SSAO::~SSAO()
{
}

void SSAO::Init()
{
    LCG lcg;

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

    for (int i = 0; i < SSAO_KERNEL_SIZE_LOW; ++i)
    {
        float3 kernel = float3::RandomDir(*rng);

        if (kernel.z < 0.0f) // flip points in lower hemishpere
            kernel.z *= -1.0f;

        float scale  = float(i) / float(SSAO_KERNEL_SIZE_LOW);
        scale        = 0.1f + (scale * scale) * (1.0f - 0.1f); // Near-origin bias
        kernel       *= lcg.Float(0.0f, 1.0f) * scale;

        kernels.push_back(kernel);
    }

    std::vector<float3> noise;
    noise.reserve(16);

    for (int i = 0; i < 16; i++)
    {
        float3 n(lcg.Float(-1.0f, 1.0f), lcg.Float(-1.0f, 1.0f), 0.0f);
        n.Normalize();
        noise.push_back(n);
    }

    if (noiseTexture == 0) glGenTextures(1, &noiseTexture);
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
