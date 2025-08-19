#include "VideoComponent.h"
#include "Application.h"
#include "ResourceTexture.h"
#include "ShaderModule.h"
#include "imgui.h"
#include <glew.h>

VideoComponent::VideoComponent(UID uid, GameObject* parent) : Component(uid, parent, "Video", COMPONENT_VIDEO)
{
    localComponentAABB                   = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
    UID videoTextureUID                  = GenerateUID();
    videoTexture                         = new ResourceTexture(videoTextureUID, "VideoTexture");

    constexpr float quadVertices[]       = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                                            1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

    constexpr unsigned int quadIndices[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

VideoComponent::VideoComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    localComponentAABB  = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));

    UID videoTextureUID = GenerateUID();
    videoTexture        = new ResourceTexture(videoTextureUID, "VideoTexture");

    if (initialState.HasMember("Video Name"))
    {
        strncpy_s(videoName, sizeof(videoName), initialState["Video Name"].GetString(), _TRUNCATE);
        videoName[sizeof(videoName) - 1] = '\0';
    }

    constexpr float quadVertices[]       = {-1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,
                                            1.0f,  1.0f,  1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f};

    constexpr unsigned int quadIndices[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

VideoComponent::~VideoComponent()
{
    ClearVideo();

    if (videoTexture)
    {
        unsigned int texture = videoTexture->GetTextureID();
        glDeleteTextures(1, &texture);
        delete videoTexture;
    }

    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void VideoComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("Video Name", rapidjson::Value(videoName, allocator), allocator);
}

void VideoComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_VIDEO)
    {
        const VideoComponent* videoComponent = static_cast<const VideoComponent*>(other);

        strncpy_s(videoName, sizeof(videoName), videoComponent->videoName, _TRUNCATE);
        videoName[sizeof(videoName) - 1] = '\0';

        if (videoTexture)
        {
            unsigned int texture = videoTexture->GetTextureID();
            glDeleteTextures(1, &texture);
            delete videoTexture;
        }

        UID videoTextureUID = GenerateUID();
        videoTexture = new ResourceTexture(videoTextureUID, "VideoTexture");
    }
}

void VideoComponent::Update(float deltaTime)
{
    timeSinceLastFrame += deltaTime;
    if (timeSinceLastFrame >= frameDelay)
    {
        if (UpdateFrame()) timeSinceLastFrame = 0.0f;
    }
}

void VideoComponent::Render(float deltaTime, CameraComponent* camera)
{
    if (!videoTexture || videoTexture->GetTextureID() == 0) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, videoTexture->GetTextureID());

    const unsigned int program = App->GetShaderModule()->GetVideoProgram();
    glUseProgram(program);

    GLint loc = glGetUniformLocation(program, "videoTexture");
    glUniform1i(loc, 0);

    // Render
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void VideoComponent::RenderDebug(float deltaTime)
{
}

void VideoComponent::RenderEditorInspector()
{
    ImGui::InputText("Video Name", videoName, IM_ARRAYSIZE(videoName));

    if (ImGui::Button("Play"))
    {
        Play();
    }

    if (ImGui::Button("Stop"))
    {
        timeSinceLastFrame  = 0.0f;
        isPlaying = false;
    }
}

void VideoComponent::ParentUpdated()
{
}

bool VideoComponent::InitVideo()
{
    const std::string path = VIDEOS_ASSETS_PATH + std::string(videoName) + VIDEOS_EXTENSION;
    if (avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr) != 0) return false;
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) return false;

    for (unsigned i = 0; i < formatCtx->nb_streams; ++i)
    {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) return false;

    AVCodecParameters* codecPar = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec        = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) return false;

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) return false;

    packet       = av_packet_alloc();
    frame        = av_frame_alloc();
    rgbFrame     = av_frame_alloc();

    int width    = codecCtx->width;
    int height   = codecCtx->height;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    frameBuffer  = (uint8_t*)av_malloc(numBytes);
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, frameBuffer, AV_PIX_FMT_RGB24, width, height, 1);

    swsCtx = sws_getContext(
        width, height, codecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    videoTexture->SetTextureID(texID);

    AVRational timeBase = formatCtx->streams[videoStreamIndex]->time_base;
    //frameDelay          = av_q2d(timeBase) * formatCtx->streams[videoStreamIndex]->avg_frame_rate.den;

    frameDelay       = 1.0 / av_q2d(formatCtx->streams[videoStreamIndex]->avg_frame_rate);

    timeSinceLastFrame  = 0.0f;
    isPlaying           = true;

    return true;
}

bool VideoComponent::UpdateFrame()
{
    if (!formatCtx || !packet || !codecCtx || !frame || !rgbFrame || !swsCtx || !videoTexture) return false;

    while (av_read_frame(formatCtx, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            if (avcodec_send_packet(codecCtx, packet) < 0) return false;
            if (avcodec_receive_frame(codecCtx, frame) >= 0)
            {
                sws_scale(
                    swsCtx, frame->data, frame->linesize, 0, codecCtx->height, rgbFrame->data, rgbFrame->linesize
                );

                glBindTexture(GL_TEXTURE_2D, videoTexture->GetTextureID());
                glTexSubImage2D(
                    GL_TEXTURE_2D, 0, 0, 0, codecCtx->width, codecCtx->height, GL_RGB, GL_UNSIGNED_BYTE,
                    rgbFrame->data[0]
                );

                av_packet_unref(packet);
                return true;
            }
        }
        av_packet_unref(packet);
    }
    isPlaying = false;
    return false;
}

void VideoComponent::ClearVideo()
{
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (rgbFrame) {
        av_free(frameBuffer);
        av_frame_free(&rgbFrame);
        rgbFrame = nullptr;
        frameBuffer = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }

    videoStreamIndex = -1;
    isPlaying = false;
    timeSinceLastFrame = 0.0f;
}

void VideoComponent::Play()
{
    if (formatCtx || codecCtx || frame || rgbFrame || packet || swsCtx) {
        ClearVideo();
    }

    InitVideo();
}
