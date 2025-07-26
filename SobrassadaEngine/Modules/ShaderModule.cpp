#include "ShaderModule.h"
#include "Application.h"
#include "DebugDrawModule.h"

#include "glew.h"

ShaderModule::ShaderModule()
{
}

ShaderModule::~ShaderModule()
{
}

bool ShaderModule::Init()
{
    specularGlossinessProgram      = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, SPECULAR_FRAGMENT_SHADER_PATH);
    specularGlossinessProgramUnlit = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, UNLIT_FRAGMENT_SHADER_PATH);

    metallicRoughnessProgram       = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, METALLIC_IBL_FRAGMENT_SHADER_PATH);
    metallicRoughnessProgramUnlit  = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, UNLIT_FRAGMENT_SHADER_PATH);

    uiWidgetProgram                = CreateShaderProgram(UIWIDGET_VERTEX_SHADER_PATH, UIWIDGET_FRAGMENT_SHADER_PATH);

    metallicGeometryPassProgram = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, GBUFFER_METALLIC_FRAGMENT_SHADER_PATH);
    specularGeometryPassProgram = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, GBUFFER_SPECULAR_FRAGMENT_SHADER_PATH);
    transparentPassProgram      = CreateShaderProgram(LIGHTS_VERTEX_SHADER_PATH, TRANSPARENT_FRAGMENT_SHADER_PATH);
    lightingPassProgram         = CreateShaderProgram(QUAD_VERTEX_SHADER_PATH, LIGHTINGPASS_FRAGMENT_SHADER_PATH);

    quadProgram                 = CreateShaderProgram(QUAD_VERTEX_SHADER_PATH, QUAD_FRAGMENT_SHADER_PATH);
    depthProgram                = CreateShaderProgram(QUAD_VERTEX_SHADER_PATH, DEPTH_FRAGMENT_SHADER_PATH);
    linearDepthProgram          = CreateShaderProgram(QUAD_VERTEX_SHADER_PATH, LINEARDEPTH_FRAGMENT_SHADER_PATH);
    billboardProgram            = CreateShaderProgram(BILLBOARD_VERTEX_SHADER_PATH, BILLBOARD_FRAGMENT_SHADER_PATH);
    trailProgram                = CreateShaderProgram(TRAIL_VERTEX_SHADER_PATH, TRAIL_FRAGMENT_SHADER_PATH);
    decalProgram                = CreateShaderProgram(DECAL_VERTEX_SHADER_PATH, DECAL_FRAGMENT_SHADER_PATH);

    shadowMapProgram            = CreateShaderProgram(SHADOWMAP_VERTEX_SHADER_PATH, EMPTY_FRAGMENT_SHADER_PATH);

    shadowDepthProgram          = CreateComputeProgram(SHADOW_DEPTH_COMPUTE_SHADER_PATH);
    tileShadingProgram          = CreateComputeProgram(TILE_SHADING_COMPUTE_SHADER_PATH);

    spritesheetProgram          = CreateShaderProgram(SPRITESHEET_VERTEX_SHADER_PATH, SPRITESHEET_FRAGMENT_SHADER_PATH);
    particleSystemProgram = CreateShaderProgram(PARTICLESYSTEM_VERTEX_SHADER_PATH, PARTICLESYSTEM_FRAGMENT_SHADER_PATH);
    return true;
}

bool ShaderModule::ShutDown()
{
    glDeleteProgram(specularGlossinessProgram);
    glDeleteProgram(specularGlossinessProgramUnlit);
    glDeleteProgram(metallicRoughnessProgram);
    glDeleteProgram(metallicRoughnessProgramUnlit);
    glDeleteProgram(uiWidgetProgram);
    glDeleteProgram(metallicGeometryPassProgram);
    glDeleteProgram(specularGeometryPassProgram);
    glDeleteProgram(lightingPassProgram);
    glDeleteProgram(quadProgram);
    glDeleteProgram(depthProgram);
    glDeleteProgram(linearDepthProgram);
    glDeleteProgram(billboardProgram);
    glDeleteProgram(decalProgram);
    glDeleteProgram(trailProgram);
    glDeleteProgram(shadowMapProgram);
    glDeleteProgram(shadowDepthProgram);
    glDeleteProgram(tileShadingProgram);
    glDeleteProgram(spritesheetProgram);
    glDeleteProgram(particleSystemProgram);

    return true;
}

unsigned int ShaderModule::CreateShaderProgram(const char* vertexPath, const char* fragmentPath)
{
    // GLOG("Loading shaders")
    unsigned int program    = 0;

    char* vertexShader      = LoadShaderSource(vertexPath);
    char* fragmentShader    = LoadShaderSource(fragmentPath);

    unsigned int vertexId   = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fragmentId = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    program                 = CreateProgram(vertexId, fragmentId);

    free(vertexShader);
    free(fragmentShader);

    return program;
}

unsigned int ShaderModule::CreateComputeProgram(const char* computePath)
{
    unsigned int program   = 0;

    char* computeShader    = LoadShaderSource(computePath);

    unsigned int computeId = CompileShader(GL_COMPUTE_SHADER, computeShader);

    program                = CreateCompProgram(computeId);

    free(computeShader);

    return program;
}

char* ShaderModule::LoadShaderSource(const char* shaderPath)
{
    // GLOG("Reading shader: %s", shaderPath)
    char* data = nullptr;
    FILE* file = nullptr;

    fopen_s(&file, shaderPath, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        data     = (char*)malloc(size + 1);
        fseek(file, 0, SEEK_SET);
        fread(data, 1, size, file);
        data[size] = 0;
        fclose(file);
    }

    return data;
}

unsigned int ShaderModule::CompileShader(unsigned int shaderType, const char* source)
{
    // GLOG("Compiling %s", GL_VERTEX_SHADER == shaderType ? "vertex shader" : "fragment shader")
    unsigned shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &source, 0);
    glCompileShader(shaderId);

    int compileSuccess = GL_FALSE;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileSuccess);

    if (compileSuccess == GL_FALSE)
    {
        GLOG("Error compiling %s", GL_VERTEX_SHADER == shaderType ? "vertex shader" : "fragment shader")
        int logLenght = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLenght);
        if (logLenght > 0)
        {
            int written = 0;
            char* info  = (char*)malloc(logLenght);
            glGetShaderInfoLog(shaderId, logLenght, &written, info);
            GLOG("Log Info: %s", info);
            free(info);
        }
    }

    return shaderId;
}

unsigned int ShaderModule::CreateProgram(unsigned int vertexShader, unsigned int fragmentShader)
{
    // GLOG("Creating shader program")
    unsigned programId = glCreateProgram();
    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);

    int compileSuccess = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &compileSuccess);

    if (compileSuccess == GL_FALSE)
    {
        GLOG("Error creating shader program")
        int logLenght = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLenght);

        if (logLenght > 0)
        {
            int written = 0;
            char* info  = (char*)malloc(logLenght);
            glGetProgramInfoLog(programId, logLenght, &written, info);
            GLOG("Program Log Info: %s", info);
            free(info);
        }
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programId;
}

unsigned int ShaderModule::CreateCompProgram(unsigned int computeShader)
{
    unsigned programId = glCreateProgram();
    glAttachShader(programId, computeShader);
    glLinkProgram(programId);

    int compileSuccess = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &compileSuccess);

    if (compileSuccess == GL_FALSE)
    {
        GLOG("Error creating shader program")
        int logLenght = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLenght);

        if (logLenght > 0)
        {
            int written = 0;
            char* info  = (char*)malloc(logLenght);
            glGetProgramInfoLog(programId, logLenght, &written, info);
            GLOG("Program Log Info: %s", info);
            free(info);
        }
    }

    glDeleteShader(computeShader);

    return programId;
}

void ShaderModule::DeleteProgram(unsigned int programID)
{
    glDeleteProgram(programID);
}

int ShaderModule::GetSpecularGlossinessProgram() const
{

    return App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_LIGTHS)
             ? specularGlossinessProgram
             : specularGlossinessProgramUnlit;
}

int ShaderModule::GetMetallicRoughnessProgram() const
{
    return App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_LIGTHS)
             ? metallicRoughnessProgram
             : metallicRoughnessProgramUnlit;
}

int ShaderModule::GetMetallicGeometryPassProgram() const
{
    return App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_LIGTHS)
             ? metallicGeometryPassProgram
             : metallicGeometryPassProgram;
}

int ShaderModule::GetSpecularGeometryPassProgram() const
{
    return App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_LIGTHS)
             ? specularGeometryPassProgram
             : specularGeometryPassProgram;
}

int ShaderModule::GetLightingPassProgram() const
{
    return App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_LIGTHS) ? lightingPassProgram
                                                                                            : lightingPassProgram;
}