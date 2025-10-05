#include "OpenGLModule.h"

#include "Application.h"
#include "Framebuffer.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "WindowModule.h"

#include "glew.h"
#include <Windows.h>

#ifdef OPTICK
#include "optick.h"
#endif

OpenGLModule::OpenGLModule()
{
}

OpenGLModule::~OpenGLModule()
{
}

void __stdcall OurOpenGLErrorFunction(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam
)
{
    const char *tmp_source = "", *tmp_type = "", *tmp_severity = "";
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        tmp_source = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        tmp_source = "Window System";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        tmp_source = "Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        tmp_source = "Third Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        tmp_source = "Application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        tmp_source = "Other";
        break;
    };
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        tmp_type = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        tmp_type = "Deprecated Behaviour";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        tmp_type = "Undefined Behaviour";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        tmp_type = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        tmp_type = "Performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        tmp_type = "Marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        tmp_type = "Push Group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        tmp_type = "Pop Group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        tmp_type = "Other";
        break;
    };
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        tmp_severity = "high";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        tmp_severity = "medium";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        tmp_severity = "low";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        tmp_severity = "notification";
        break;
    };
    GLOG("<Source:%s> <Type:%s> <Severity:%s> <ID:%d> <Message:%s>\n", tmp_source, tmp_type, tmp_severity, id, message);
}

bool OpenGLModule::Init()
{
    // GLOG("Creating Renderer context");

    context    = SDL_GL_CreateContext(App->GetWindowModule()->window);
    GLenum err = glewInit();

    // GLOG("Using Glew %s", glewGetString(GLEW_VERSION));

    // GLOG("Vendor: %s", glGetString(GL_VENDOR));
    // GLOG("Renderer: %s", glGetString(GL_RENDERER));
    // GLOG("OpenGL version supported %s", glGetString(GL_VERSION));
    // GLOG("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glEnable(GL_DEPTH_TEST);                // Enable depth test
    glEnable(GL_CULL_FACE);                 // Enable cull backward faces
    glFrontFace(GL_CCW);                    // Enable conter clock wise backward faces
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // Enable seamless cubemap

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);             // Enable output callbacks
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Output callbacks

    glDebugMessageCallback(&OurOpenGLErrorFunction, nullptr);
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE
    ); // Filter notification
    glDebugMessageControl(
        GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE
    ); // Filter Low with Other (like another notification message)
#endif

    // stencil op
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    framebuffer = new Framebuffer(App->GetWindowModule()->GetWidth(), App->GetWindowModule()->GetHeight());
    gBuffer     = new GBuffer(App->GetWindowModule()->GetWidth(), App->GetWindowModule()->GetHeight());
    ssao        = new SSAO(App->GetWindowModule()->GetWidth(), App->GetWindowModule()->GetHeight());

    WindowModule* windowModule = App->GetWindowModule();
    windowModule->SetVsync(windowModule->GetVsync());

    return true;
}

update_status OpenGLModule::PreUpdate(float deltaTime)
{
    int CurrentWidth  = 0;
    int CurrentHeight = 0;
    SDL_GetWindowSize(App->GetWindowModule()->window, &CurrentWidth, &CurrentHeight);

#ifndef GAME
    framebuffer->Bind();
#endif

    if (CurrentWidth && CurrentHeight)
    {
#ifndef GAME
        glViewport(0, 0, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());
#else
        glViewport(0, 0, CurrentWidth, CurrentHeight);
#endif
        glClearColor(clearColorRed, clearColorGreen, clearColorBlue, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    drawCallsCount     = 0;
    verticesCount      = 0;
    trianglesPerSecond = 0;

    return UPDATE_CONTINUE;
}

update_status OpenGLModule::Update(float deltaTime)
{
    return UPDATE_CONTINUE;
}

update_status OpenGLModule::PostUpdate(float deltaTime)
{
#ifdef OPTICK
    OPTICK_CATEGORY("OpengGLModule::PostUpdate", Optick::Category::Wait)
#endif
    {

        framebuffer->CheckResize();
        gBuffer->CheckResize();
        ssao->CheckResize();

#ifdef OPTICK
        OPTICK_CATEGORY("OpengGLModule::PostUpdate_SwapWindow", Optick::Category::Wait)
#endif
        SDL_GL_SwapWindow(App->GetWindowModule()->window);
    }

    return UPDATE_CONTINUE;
}

bool OpenGLModule::ShutDown()
{
    // GLOG("Destroying renderer");

    SDL_GL_DeleteContext(App->GetWindowModule()->window);
    delete framebuffer;
    delete gBuffer;
    delete ssao;
    return true;
}

void OpenGLModule::DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    glDrawElements(mode, count, type, indices);
    drawCallsCount++;
}

void OpenGLModule::DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    glDrawArrays(mode, first, count);
    drawCallsCount++;
}

void OpenGLModule::SetRenderWireframe(bool renderWireframe)
{
    if (renderWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void OpenGLModule::SetDepthTest(bool enable)
{
    if (enable) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void OpenGLModule::SetFaceCull(bool enable)
{
    if (enable) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void OpenGLModule::SetDepthFunc(bool enable)
{
    if (enable) glDepthFunc(GL_LESS);
    else glDepthFunc(GL_ALWAYS);
}

void OpenGLModule::SetFrontFaceMode(int mode)
{
    glFrontFace(mode);
}
