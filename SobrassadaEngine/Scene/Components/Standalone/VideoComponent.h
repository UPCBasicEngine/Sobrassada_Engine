#pragma once

#include "Component.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class ResourceTexture;

class VideoComponent : public Component
{
  public:
    VideoComponent(UID uid, GameObject* parent);
    VideoComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~VideoComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* camera) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void ParentUpdated() override;

    void ClearVideo();
    void SOBRASADA_API_ENGINE Play();
    bool SOBRASADA_API_ENGINE IsPlaying() const { return isPlaying; }

  private:
    bool InitVideo();
    bool UpdateFrame();

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx   = nullptr;
    AVFrame* frame             = nullptr;
    AVFrame* rgbFrame          = nullptr;
    AVPacket* packet           = nullptr;
    SwsContext* swsCtx         = nullptr;

    int videoStreamIndex       = -1;
    uint8_t* frameBuffer       = nullptr;

    // Texture2D* videoTexture = nullptr;
    float accumulatedTime      = 0.0f;
    float frameDelay           = 0.0f;

    char videoName[64]         = "";
    ResourceTexture* videoTexture;

    float timeSinceLastFrame = 0.0f;

    unsigned int VAO, VBO, EBO;

    bool isPlaying = false;
};