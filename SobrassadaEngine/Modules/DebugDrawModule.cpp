#include "DebugDrawModule.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DynamicOctree.h"
#include "Framebuffer.h"
#include "GameObject.h"
#include "Globals.h"
#include "MathGeoLib.h"
#include "Octree.h"
#include "OpenGLModule.h"
#include "PathfinderModule.h"
#include "Quadtree.h"
#include "ResourceNavmesh.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/SplineComponent.h"

#include "SDL_video.h"
#define DEBUG_DRAW_IMPLEMENTATION
#include "DebugDraw.h" // Debug Draw API. Notice that we need the DEBUG_DRAW_IMPLEMENTATION macro here!
#include "Geometry/AABB.h"
#include "Geometry/OBB.h"
#include "btVector3.h"
#include "glew.h"
#include "imgui.h"

class DDRenderInterfaceCoreGL final : public dd::RenderInterface
{
  public:
    //
    // dd::RenderInterface overrides:
    //

    void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override
    {
        assert(points != nullptr);
        assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

        glBindVertexArray(linePointVAO);
        glUseProgram(linePointProgram);

        glUniformMatrix4fv(linePointProgram_MvpMatrixLocation, 1, GL_TRUE, reinterpret_cast<float*>(&mvpMatrix));

        bool already = glIsEnabled(GL_DEPTH_TEST);

        if (depthEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
        glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), points);

        // Issue the draw call:
        App->GetOpenGLModule()->DrawArrays(GL_POINTS, 0, count);

        glUseProgram(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        checkGLError(__FILE__, __LINE__);

        if (already)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override
    {
        assert(lines != nullptr);
        assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

        glBindVertexArray(linePointVAO);
        glUseProgram(linePointProgram);

        glUniformMatrix4fv(linePointProgram_MvpMatrixLocation, 1, GL_TRUE, reinterpret_cast<const float*>(&mvpMatrix));

        bool already = glIsEnabled(GL_DEPTH_TEST);

        if (depthEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
        glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), lines);

        // Issue the draw call:
        App->GetOpenGLModule()->DrawArrays(GL_LINES, 0, count);

        glUseProgram(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        checkGLError(__FILE__, __LINE__);

        if (already)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override
    {
        assert(glyphs != nullptr);
        assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

        glBindVertexArray(textVAO);
        glUseProgram(textProgram);

        // These doesn't have to be reset every draw call, I'm just being lazy ;)
        glUniform1i(textProgram_GlyphTextureLocation, 0);
        glUniform2f(textProgram_ScreenDimensions, static_cast<GLfloat>(width), static_cast<GLfloat>(height));

        if (glyphTex != nullptr)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, handleToGL(glyphTex));
        }

        bool already_blend = glIsEnabled(GL_BLEND);

        if (!already_blend)
        {
            glEnable(GL_BLEND);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        bool already = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);

        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), glyphs);

        // Issue the draw call
        App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, count);

        if (!already_blend)
        {
            glDisable(GL_BLEND);
        }

        glUseProgram(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        checkGLError(__FILE__, __LINE__);

        if (already)
        {
            glEnable(GL_DEPTH_TEST);
        }
    }

    dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void* pixels) override
    {
        assert(width > 0 && height > 0);
        assert(pixels != nullptr);

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        checkGLError(__FILE__, __LINE__);

        return GLToHandle(textureId);
    }

    void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override
    {
        if (glyphTex == nullptr)
        {
            return;
        }

        const GLuint textureId = handleToGL(glyphTex);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureId);
    }

    // These two can also be implemented to perform GL render
    // state setup/cleanup, but we don't use them in this sample.
    // void beginDraw() override { }
    // void endDraw()   override { }

    //
    // Local methods:
    //

    DDRenderInterfaceCoreGL()
        : mvpMatrix(), width(0), height(0), linePointProgram(0), linePointProgram_MvpMatrixLocation(-1), textProgram(0),
          textProgram_GlyphTextureLocation(-1), textProgram_ScreenDimensions(-1), linePointVAO(0), linePointVBO(0),
          textVAO(0), textVBO(0)
    {
        // std::printf("\n");
        // std::printf("GL_VENDOR    : %s\n",   glGetString(GL_VENDOR));
        // std::printf("GL_RENDERER  : %s\n",   glGetString(GL_RENDERER));
        // std::printf("GL_VERSION   : %s\n",   glGetString(GL_VERSION));
        // std::printf("GLSL_VERSION : %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        // std::printf("DDRenderInterfaceCoreGL initializing ...\n");

        // Default OpenGL states:
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // This has to be enabled since the point drawing shader will use gl_PointSize.
        glEnable(GL_PROGRAM_POINT_SIZE);

        setupShaderPrograms();
        setupVertexBuffers();

        // std::printf("DDRenderInterfaceCoreGL ready!\n\n");
    }

    ~DDRenderInterfaceCoreGL()
    {
        glDeleteProgram(linePointProgram);
        glDeleteProgram(textProgram);

        glDeleteVertexArrays(1, &linePointVAO);
        glDeleteBuffers(1, &linePointVBO);

        glDeleteVertexArrays(1, &textVAO);
        glDeleteBuffers(1, &textVBO);
    }

    void setupShaderPrograms()
    {
        // std::printf("> DDRenderInterfaceCoreGL::setupShaderPrograms()\n");

        //
        // Line/point drawing shader:
        //
        {
            GLuint linePointVS = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(linePointVS, 1, &linePointVertShaderSrc, nullptr);
            compileShader(linePointVS);

            GLint linePointFS = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(linePointFS, 1, &linePointFragShaderSrc, nullptr);
            compileShader(linePointFS);

            linePointProgram = glCreateProgram();
            glAttachShader(linePointProgram, linePointVS);
            glAttachShader(linePointProgram, linePointFS);

            glBindAttribLocation(linePointProgram, 0, "in_Position");
            glBindAttribLocation(linePointProgram, 1, "in_ColorPointSize");
            linkProgram(linePointProgram);

            linePointProgram_MvpMatrixLocation = glGetUniformLocation(linePointProgram, "u_MvpMatrix");
            if (linePointProgram_MvpMatrixLocation < 0)
            {
                // errorF("Unable to get u_MvpMatrix uniform location!");
            }
            checkGLError(__FILE__, __LINE__);
        }

        //
        // Text rendering shader:
        //
        {
            GLuint textVS = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(textVS, 1, &textVertShaderSrc, nullptr);
            compileShader(textVS);

            GLint textFS = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(textFS, 1, &textFragShaderSrc, nullptr);
            compileShader(textFS);

            textProgram = glCreateProgram();
            glAttachShader(textProgram, textVS);
            glAttachShader(textProgram, textFS);

            glBindAttribLocation(textProgram, 0, "in_Position");
            glBindAttribLocation(textProgram, 1, "in_TexCoords");
            glBindAttribLocation(textProgram, 2, "in_Color");
            linkProgram(textProgram);

            textProgram_GlyphTextureLocation = glGetUniformLocation(textProgram, "u_glyphTexture");
            if (textProgram_GlyphTextureLocation < 0)
            {
                // errorF("Unable to get u_glyphTexture uniform location!");
            }

            textProgram_ScreenDimensions = glGetUniformLocation(textProgram, "u_screenDimensions");
            if (textProgram_ScreenDimensions < 0)
            {
                // errorF("Unable to get u_screenDimensions uniform location!");
            }

            checkGLError(__FILE__, __LINE__);
        }
    }

    void setupVertexBuffers()
    {
        // std::printf("> DDRenderInterfaceCoreGL::setupVertexBuffers()\n");

        //
        // Lines/points vertex buffer:
        //
        {
            glGenVertexArrays(1, &linePointVAO);
            glGenBuffers(1, &linePointVBO);
            checkGLError(__FILE__, __LINE__);

            glBindVertexArray(linePointVAO);
            glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);

            // RenderInterface will never be called with a batch larger than
            // DEBUG_DRAW_VERTEX_BUFFER_SIZE vertexes, so we can allocate the same amount here.
            glBufferData(
                GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW
            );
            checkGLError(__FILE__, __LINE__);

            // Set the vertex format expected by 3D points and lines:
            std::size_t offset = 0;

            glEnableVertexAttribArray(0); // in_Position (vec3)
            glVertexAttribPointer(
                /* index     = */ 0,
                /* size      = */ 3,
                /* type      = */ GL_FLOAT,
                /* normalize = */ GL_FALSE,
                /* stride    = */ sizeof(dd::DrawVertex),
                /* offset    = */ reinterpret_cast<void*>(offset)
            );
            offset += sizeof(float) * 3;

            glEnableVertexAttribArray(1); // in_ColorPointSize (vec4)
            glVertexAttribPointer(
                /* index     = */ 1,
                /* size      = */ 4,
                /* type      = */ GL_FLOAT,
                /* normalize = */ GL_FALSE,
                /* stride    = */ sizeof(dd::DrawVertex),
                /* offset    = */ reinterpret_cast<void*>(offset)
            );

            checkGLError(__FILE__, __LINE__);

            // VAOs can be a pain in the neck if left enabled...
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        //
        // Text rendering vertex buffer:
        //
        {
            glGenVertexArrays(1, &textVAO);
            glGenBuffers(1, &textVBO);
            checkGLError(__FILE__, __LINE__);

            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);

            // NOTE: A more optimized implementation might consider combining
            // both the lines/points and text buffers to save some memory!
            glBufferData(
                GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW
            );
            checkGLError(__FILE__, __LINE__);

            // Set the vertex format expected by the 2D text:
            std::size_t offset = 0;

            glEnableVertexAttribArray(0); // in_Position (vec2)
            glVertexAttribPointer(
                /* index     = */ 0,
                /* size      = */ 2,
                /* type      = */ GL_FLOAT,
                /* normalize = */ GL_FALSE,
                /* stride    = */ sizeof(dd::DrawVertex),
                /* offset    = */ reinterpret_cast<void*>(offset)
            );
            offset += sizeof(float) * 2;

            glEnableVertexAttribArray(1); // in_TexCoords (vec2)
            glVertexAttribPointer(
                /* index     = */ 1,
                /* size      = */ 2,
                /* type      = */ GL_FLOAT,
                /* normalize = */ GL_FALSE,
                /* stride    = */ sizeof(dd::DrawVertex),
                /* offset    = */ reinterpret_cast<void*>(offset)
            );
            offset += sizeof(float) * 2;

            glEnableVertexAttribArray(2); // in_Color (vec4)
            glVertexAttribPointer(
                /* index     = */ 2,
                /* size      = */ 4,
                /* type      = */ GL_FLOAT,
                /* normalize = */ GL_FALSE,
                /* stride    = */ sizeof(dd::DrawVertex),
                /* offset    = */ reinterpret_cast<void*>(offset)
            );

            checkGLError(__FILE__, __LINE__);

            // Ditto.
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    static GLuint handleToGL(dd::GlyphTextureHandle handle)
    {
        const std::size_t temp = reinterpret_cast<std::size_t>(handle);
        return static_cast<GLuint>(temp);
    }

    static dd::GlyphTextureHandle GLToHandle(const GLuint id)
    {
        const std::size_t temp = static_cast<std::size_t>(id);
        return reinterpret_cast<dd::GlyphTextureHandle>(temp);
    }

    static void checkGLError(const char* file, const int line)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            // errorF("%s(%d) : GL_CORE_ERROR=0x%X - %s", file, line, err, errorToString(err));
        }
    }

    static void compileShader(const GLuint shader)
    {
        glCompileShader(shader);
        checkGLError(__FILE__, __LINE__);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        checkGLError(__FILE__, __LINE__);

        if (status == GL_FALSE)
        {
            GLchar strInfoLog[1024] = {0};
            glGetShaderInfoLog(shader, sizeof(strInfoLog) - 1, nullptr, strInfoLog);
            // errorF("\n>>> Shader compiler errors:\n%s", strInfoLog);
        }
    }

    static void linkProgram(const GLuint program)
    {
        glLinkProgram(program);
        checkGLError(__FILE__, __LINE__);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        checkGLError(__FILE__, __LINE__);

        if (status == GL_FALSE)
        {
            GLchar strInfoLog[1024] = {0};
            glGetProgramInfoLog(program, sizeof(strInfoLog) - 1, nullptr, strInfoLog);
            // errorF("\n>>> Program linker errors:\n%s", strInfoLog);
        }
    }

    // The "model-view-projection" matrix for the scene.
    // In this demo, it consists of the camera's view and projection matrices only.
    math::float4x4 mvpMatrix;
    unsigned width, height;

  private:
    GLuint linePointProgram;
    GLint linePointProgram_MvpMatrixLocation;

    GLuint textProgram;
    GLint textProgram_GlyphTextureLocation;
    GLint textProgram_ScreenDimensions;

    GLuint linePointVAO;
    GLuint linePointVBO;

    GLuint textVAO;
    GLuint textVBO;

    static const char* linePointVertShaderSrc;
    static const char* linePointFragShaderSrc;

    static const char* textVertShaderSrc;
    static const char* textFragShaderSrc;

}; // class DDRenderInterfaceCoreGL

// ========================================================
// Minimal shaders we need for the debug primitives:
// ========================================================

const char* DDRenderInterfaceCoreGL::linePointVertShaderSrc =
    "\n"
    "#version 150\n"
    "\n"
    "in vec3 in_Position;\n"
    "in vec4 in_ColorPointSize;\n"
    "\n"
    "out vec4 v_Color;\n"
    "uniform mat4 u_MvpMatrix;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position  = u_MvpMatrix * vec4(in_Position, 1.0);\n"
    "    gl_PointSize = in_ColorPointSize.w;\n"
    "    v_Color      = vec4(in_ColorPointSize.xyz, 1.0);\n"
    "}\n";

const char* DDRenderInterfaceCoreGL::linePointFragShaderSrc = "\n"
                                                              "#version 150\n"
                                                              "\n"
                                                              "in  vec4 v_Color;\n"
                                                              "out vec4 out_FragColor;\n"
                                                              "\n"
                                                              "void main()\n"
                                                              "{\n"
                                                              "    out_FragColor = v_Color;\n"
                                                              "}\n";

const char* DDRenderInterfaceCoreGL::textVertShaderSrc =
    "\n"
    "#version 150\n"
    "\n"
    "in vec2 in_Position;\n"
    "in vec2 in_TexCoords;\n"
    "in vec3 in_Color;\n"
    "\n"
    "uniform vec2 u_screenDimensions;\n"
    "\n"
    "out vec2 v_TexCoords;\n"
    "out vec4 v_Color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Map to normalized clip coordinates:\n"
    "    float x = ((2.0 * (in_Position.x - 0.5)) / u_screenDimensions.x) - 1.0;\n"
    "    float y = 1.0 - ((2.0 * (in_Position.y - 0.5)) / u_screenDimensions.y);\n"
    "\n"
    "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
    "    v_TexCoords = in_TexCoords;\n"
    "    v_Color     = vec4(in_Color, 1.0);\n"
    "}\n";

const char* DDRenderInterfaceCoreGL::textFragShaderSrc =
    "\n"
    "#version 150\n"
    "\n"
    "in vec2 v_TexCoords;\n"
    "in vec4 v_Color;\n"
    "\n"
    "uniform sampler2D u_glyphTexture;\n"
    "out vec4 out_FragColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    out_FragColor = v_Color;\n"
    "    out_FragColor.a = texture(u_glyphTexture, v_TexCoords).r;\n"
    "}\n";

DDRenderInterfaceCoreGL* DebugDrawModule::implementation = 0;

DebugDrawModule::DebugDrawModule()
{
}

DebugDrawModule::~DebugDrawModule()
{
}

bool DebugDrawModule::Init()
{
    implementation = new DDRenderInterfaceCoreGL;
    dd::initialize(implementation);

    debugOptionValues.set((int)DebugOptions::RENDER_LIGTHS);

    return true;
}

bool DebugDrawModule::ShutDown()
{
    dd::shutdown();

    delete implementation;
    implementation = 0;

    return true;
}

update_status DebugDrawModule::Render(float deltaTime)
{
// dd::axisTriad(float4x4::identity, 0.1f, 1.0f);
#ifndef GAME
    if (!App->GetSceneModule()->GetInPlayMode()) dd::xzSquareGrid(-10, 10, 0.0f, 1.0f, dd::colors::Blue);
#endif

    if (App->GetSceneModule()->GetInPlayMode())
    {
        CameraComponent* camera    = App->GetSceneModule()->GetScene()->GetMainCamera();

        const float4x4& viewMatrix = camera ? camera->GetViewMatrix() : App->GetCameraModule()->GetViewMatrix();
        const float4x4& projectionMatrix =
            camera ? camera->GetProjectionMatrix() : App->GetCameraModule()->GetProjectionMatrix();

        auto framebuffer = App->GetOpenGLModule()->GetFramebuffer();
        const int width  = framebuffer->GetTextureWidth();
        const int height = framebuffer->GetTextureHeight();
        Draw(viewMatrix, projectionMatrix, width, height);
    }
    else Draw();
    // Probably should go somewhere else, but must go after skybox and meshes

    return UPDATE_CONTINUE;
}

void DebugDrawModule::Draw()
{
    CameraModule* cameraModule = App->GetCameraModule();

    const float4x4& projection = cameraModule->GetProjectionMatrix();
    const float4x4& view       = cameraModule->GetViewMatrix();
    int width                  = 0;
    int height                 = 0;

    if (cameraModule->IsCameraDetached())
    {
        float4x4 frustumProj       = cameraModule->GetFrustumProjectionMatrix();
        float4x4 frustumView       = cameraModule->GetFrustumViewMatrix();
        float4x4 inverseClipMatrix = frustumProj * frustumView;
        inverseClipMatrix.Inverse();

        dd::frustum(inverseClipMatrix, float3(1.f, 1.f, 1.f));
    }

    if (App->GetSceneModule()->IsSceneLoaded()) HandleDebugRenderOptions();

    auto framebuffer          = App->GetOpenGLModule()->GetFramebuffer();
    width                     = framebuffer->GetTextureWidth();
    height                    = framebuffer->GetTextureHeight();

    implementation->width     = width;
    implementation->height    = height;
    implementation->mvpMatrix = projection * view;

    dd::flush();
}

void DebugDrawModule::Render2DLines(const std::vector<float4>& lines, const float3& color, float depth)
{
    for (auto& line : lines)
    {
        dd::line(ddVec3(line.x, line.y, depth), ddVec3(line.z, line.w, depth), ddVec3(color.x, color.y, color.z));
    }
}

void DebugDrawModule::RenderLines(const std::vector<LineSegment>& lines, const float3& color)
{
    for (auto& line : lines)
    {
        dd::line(line.a, line.b, color);
    }
}

void DebugDrawModule::DrawLineSegment(const LineSegment& line, const float3& color)
{
    dd::line(line.a, line.b, color);
}

void DebugDrawModule::DrawLine(
    const float3& origin, const float3& direction, const float distance, const float3& color, bool enableDepth
)
{
    float3 dir = direction.Normalized() * distance;
    dd::line(origin, dir + origin, color, 0, enableDepth);
}

void DebugDrawModule::DrawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    dd::line(
        float3(from.x(), from.y(), from.z()), float3(to.x(), to.y(), to.z()), float3(color.x(), color.y(), color.z())
    );
}

void DebugDrawModule::DrawCircle(const float3& center, const float3& upVector, const float3& color, const float radius)
{
    dd::circle(center, upVector, color, radius, 40);
}

void DebugDrawModule::DrawSphere(const float3& center, const float3& color, const float radius)
{
    dd::sphere(center, color, radius);
}

void DebugDrawModule::DrawFrustrum(float4x4 frustumProj, float4x4 frustumView)
{
    float4x4 inverseClipMatrix = frustumProj * frustumView;
    inverseClipMatrix.Inverse();
    dd::frustum(inverseClipMatrix, float3(1.f, 1.f, 1.f));
}

void DebugDrawModule::Draw(const float4x4& view, const float4x4& proj, unsigned width, unsigned height)
{

    if (App->GetSceneModule()->IsSceneLoaded()) HandleDebugRenderOptions();

    implementation->width     = width;
    implementation->height    = height;
    implementation->mvpMatrix = proj * view;

    dd::flush();
}

void DebugDrawModule::DrawAxisTriad(const float4x4& transform, bool depthEnabled)
{
    dd::axisTriad(transform, 0.005f, 0.05f, 0, depthEnabled);
}

void DebugDrawModule::DrawCross(const float3& position, const float length)
{
    dd::cross(position, length);
}

void DebugDrawModule::DrawPoint(const float3& position, const float size)
{
    dd::point(position, float3(1, 1, 1), size);
}

void DebugDrawModule::DrawCone(const float3& center, const float3& dir, const float baseRadius, const float apexRadius)
{
    dd::cone(center, dir, float3(1, 1, 1), baseRadius, apexRadius);
}

void DebugDrawModule::DrawArrow(const float3& from, const float3& to, const float3& color, float headSize)
{
    dd::arrow(from, to, color, headSize);
}

void DebugDrawModule::DrawCone(
    const float3& apexPosition, const float3& direction, const float3& color, float apexRadius, float baseRadius
)
{
    dd::cone(apexPosition, direction, color, baseRadius, apexRadius);
}

void DebugDrawModule::Draw3DText(const btVector3& location, const char* textString)
{
    bool playMode              = App->GetSceneModule()->GetInPlayMode();

    CameraModule* cameraModule = App->GetCameraModule();
    CameraComponent* camera    = App->GetSceneModule()->GetScene()->GetMainCamera();

    const float4x4& projection = playMode ? camera->GetProjectionMatrix() : cameraModule->GetProjectionMatrix();
    const float4x4& view       = playMode ? camera->GetViewMatrix() : cameraModule->GetViewMatrix();
    const float3 pos           = float3(location);

    auto framebuffer           = App->GetOpenGLModule()->GetFramebuffer();
    int width                  = framebuffer->GetTextureWidth();
    int height                 = framebuffer->GetTextureHeight();

    dd::projectedText(textString, pos, float3::zero, view * projection, 0, 0, width, height, 10.0f);
}

void DebugDrawModule::Draw2DText(const char* textString, const float3& location, const float3& color, float scale)
{
    dd::screenText(textString, location, color, scale);
}

void DebugDrawModule::DrawContactPoint(
    const btVector3& PointOnB, const btVector3& normalOnB, float distance, int lifeTime, const btVector3& color
)
{
    dd::vertexNormal(float3(PointOnB), float3(normalOnB), distance, lifeTime);
}

void DebugDrawModule::HandleDebugRenderOptions()
{
    SceneModule* sceneModule   = App->GetSceneModule();
    CameraModule* cameraModule = App->GetCameraModule();

    const auto& gameObjects    = sceneModule->GetScene()->GetAllGameObjects();

    if (debugOptionValues[(int)DebugOptions::RENDER_AABB])
    {
        for (const auto& gameObject : gameObjects)
        {
            for (int i = 0; i < 12; ++i)
                DrawLineSegment(gameObject.second->GetGlobalAABB().Edge(i), float3(0.f, 0.5f, 0.5f));
        }
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_OBB])
    {
        for (const auto& gameObject : gameObjects)
        {
            for (int i = 0; i < 12; ++i)
                DrawLineSegment(gameObject.second->GetGlobalOBB().Edge(i), float3(0.f, 1.f, 0.f));
        }
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_STATICTREE])
    {
        Octree* staticTree = sceneModule->GetScene()->GetOctree();
        if (staticTree != nullptr) RenderLines(staticTree->GetDrawLines(), float3(1.f, 0.f, 0.f));
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_DYNAMICTREE])
    {
        //DynamicOctree* dynamicTree = sceneModule->GetScene()->GetDynamicOctree();
        Octree* dynamicTree = sceneModule->GetScene()->GetOctree();
        if (dynamicTree != nullptr) RenderLines(dynamicTree->GetDrawLines(), float3(0.467f, 0.647f, 0.91f));
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_CAMERA_RAY])
    {
        DrawLineSegment(cameraModule->GetLastCastedRay(), float3(1.f, 1.f, 0.f));
    }
    if (debugOptionValues[(int)DebugOptions::RENDER_NAVMESH])
    {
        if (const ResourceNavMesh* navmesh = App->GetPathfinderModule()->GetNavMesh())
        {
            DrawNavMesh(
                navmesh->GetDetourNavMesh(), App->GetPathfinderModule()->GetDetourNavMeshQuery(),
                DRAWNAVMESH_COLOR_TILES
            );
        }
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_SPLINES])
    {
        for (const auto& gameObject : gameObjects)
        {
            SplineComponent* spline = gameObject.second->GetComponent<SplineComponent*>();
            if (spline) spline->RenderDebug(0.0f);
        }
    }

    if (debugOptionValues[(int)DebugOptions::RENDER_DEBUG_VISUALS])
    {
        for (const auto& gameObject : gameObjects)
        {
            if (!gameObject.second->IsGloballyEnabled()) continue;

            CharacterControllerComponent* character = gameObject.second->GetComponent<CharacterControllerComponent*>();
            if (character)
                DrawLine(
                    character->GetLastPosition() + float3(0.0f, 1.0f, 0.0f), character->GetFrontDirection(), 2.0f,
                    float3(1.0f, 0.0f, 0.0f)
                );

            AIAgentComponent* enemy = gameObject.second->GetComponent<AIAgentComponent*>();
            if (enemy)
            {
                DrawLine(
                    gameObject.second->GetGlobalTransform().TranslatePart() + float3(0.0f, 1.0f, 0.0f),
                    gameObject.second->GetGlobalTransform().WorldZ(), 2.0f, float3(1.0f, 0.0f, 0.0f)
                );
            }
        }
    }
}

static unsigned int DetourTransCol(unsigned int c, unsigned int a)
{
    return (a << 24) | (c & 0x00ffffff);
}

static int DetourBit(int a, int b)
{
    return (a & (1 << b)) >> b;
}

static unsigned int DetourRGBA(int r, int g, int b, int a)
{
    return ((unsigned int)r) | ((unsigned int)g << 8) | ((unsigned int)b << 16) | ((unsigned int)a << 24);
}

static unsigned int DetourIntToCol(int i, int a)
{
    int r = DetourBit(i, 1) + DetourBit(i, 3) * 2 + 1;
    int g = DetourBit(i, 2) + DetourBit(i, 4) * 2 + 1;
    int b = DetourBit(i, 0) + DetourBit(i, 5) * 2 + 1;
    return DetourRGBA(r * 63, g * 63, b * 63, a);
}

static unsigned int AreaToCol(unsigned int area)
{
    if (area == 0)
    {
        // Treat zero area type as default.
        return DetourRGBA(0, 192, 255, 255);
    }
    else
    {
        return DetourIntToCol(area, 255);
    }
}
void DebugDrawModule::DrawNavMesh(const dtNavMesh* navMesh, const dtNavMeshQuery* navQuery, unsigned char flags)
{
    if (!navMesh) return;

    for (int i = 0; i < navMesh->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (!tile || !tile->header) continue;

        dtPolyRef base         = navMesh->getPolyRefBase(tile);
        int tileNum            = navMesh->decodePolyIdTile(base);
        unsigned int tileColor = DetourIntToCol(tileNum, 128);

        // Iterate through each polygon in the tile
        for (int j = 0; j < tile->header->polyCount; ++j)
        {
            const dtPoly* p = &tile->polys[j];
            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

            const dtPolyDetail* pd = &tile->detailMeshes[j];

            unsigned int col;
            if (navQuery && navQuery->isInClosedList(base | (dtPolyRef)j)) col = DetourRGBA(255, 196, 0, 64);
            else col = DetourTransCol(AreaToCol(p->getArea()), 64);

            std::vector<LineSegment> lines;

            for (int k = 0; k < pd->triCount; ++k)
            {
                const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];

                float3 v[3];
                for (int l = 0; l < 3; ++l)
                {
                    if (t[l] < p->vertCount) v[l] = float3(&tile->verts[p->verts[t[l]] * 3]);
                    else v[l] = float3(&tile->detailVerts[(pd->vertBase + t[l] - p->vertCount) * 3]);
                }

                // Draw Triangle Edges
                lines.push_back(LineSegment(v[0], v[1]));
                lines.push_back(LineSegment(v[1], v[2]));
                lines.push_back(LineSegment(v[2], v[0]));
            }

            RenderLines(lines, float3(0.0f, 1.0f, 0.0f)); // Green color
        }
    }
}
