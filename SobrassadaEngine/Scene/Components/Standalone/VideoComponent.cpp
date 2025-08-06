#include "VideoComponent.h"

VideoComponent::VideoComponent(UID uid, GameObject* parent) : Component(uid, parent, "Video", COMPONENT_VIDEO)
{
}

VideoComponent::VideoComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

VideoComponent::~VideoComponent()
{
}

void VideoComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void VideoComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_VIDEO)
    {
        const VideoComponent* videoComponent = static_cast<const VideoComponent*>(other);
        
    }
}

void VideoComponent::Update(float deltaTime)
{
}

void VideoComponent::Render(float deltaTime, CameraComponent* camera)
{
}

void VideoComponent::RenderDebug(float deltaTime)
{
}

void VideoComponent::RenderEditorInspector()
{
}

void VideoComponent::ParentUpdated()
{
}

bool VideoComponent::InitVideo(const std::string& path) {
    if (avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr) != 0) return false;
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) return false;

    for (unsigned i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) return false;

    AVCodecParameters* codecPar = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) return false;

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) return false;

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();

    int width = codecCtx->width;
    int height = codecCtx->height;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    frameBuffer = (uint8_t*)av_malloc(numBytes);
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, frameBuffer, AV_PIX_FMT_RGB24, width, height, 1);

    swsCtx = sws_getContext(width, height, codecCtx->pix_fmt,
                            width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

    // Crear textura en el motor
    //videoTexture = new Texture2D(width, height, PixelFormat::RGB24); // ajusta a tu motor

    AVRational timeBase = formatCtx->streams[videoStreamIndex]->time_base;
    frameDelay = av_q2d(timeBase) * formatCtx->streams[videoStreamIndex]->avg_frame_rate.den;

    return true;
}
