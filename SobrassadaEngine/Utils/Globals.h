#pragma once

#define NOMINMAX
#include "Algorithm/Random/LCG.h"
#include <Geometry/AABB.h>
#include <stdio.h>
#include <vector>
#include <windows.h>

#ifdef SOBRASADA_ENGINE_API
#define SOBRASADA_API_ENGINE __declspec(dllexport)
#else
#define SOBRASADA_API_ENGINE __declspec(dllimport)
#endif

#pragma warning(disable : 4251)

extern SOBRASADA_API_ENGINE std::vector<char*>* Logs;
extern LCG* rng;

SOBRASADA_API_ENGINE void glog(const char file[], int line, const char* format, ...);

#define GLOG(format, ...) glog(__FILE__, __LINE__, format, __VA_ARGS__);

enum update_status
{
    UPDATE_CONTINUE = 1,
    UPDATE_STOP,
    UPDATE_ERROR,
    UPDATE_RESTART
};

// Deletes an array of buffers
#define RELEASE_ARRAY(x)                                                                                               \
    {                                                                                                                  \
        if (x != nullptr)                                                                                              \
        {                                                                                                              \
            delete[] x;                                                                                                \
            x = nullptr;                                                                                               \
        }                                                                                                              \
    }

// Configuration -----------
constexpr int SCREEN_WIDTH  = 1280;
constexpr int SCREEN_HEIGHT = 720;

constexpr bool FULLSCREEN   = false;

#ifdef GAME
constexpr bool FULL_DESKTOP = true;
#else
constexpr bool FULL_DESKTOP = false;
#endif

constexpr bool BORDERLESS               = false;
constexpr bool RESIZABLE                = true;
constexpr bool VSYNC                    = false;

constexpr const char* TITLE             = "Sobrassada Engine";
constexpr const char* ENGINE_NAME       = "Sobrassada";
constexpr const char* ORGANIZATION_NAME = "Hound of Ulster";
constexpr const char* ENGINE_VERSION    = "0.2.0";
constexpr int HFOV                      = 90;

#ifdef _WIN32
constexpr char DELIMITER = '\\';
#else
constexpr char DELIMITER = '/';
#endif

constexpr const char* FILENAME_SEPARATOR             = "_";
constexpr const char* DEFAULT_SCENE_NAME             = "New Scene";
constexpr const char* DEFAULT_NODE_NAME              = "Unnamed Node";
constexpr const char* DEFAULT_NAVMESH_NAME           = "New Navmesh";

constexpr float DEFAULT_GL_CLEAR_COLOR_RED           = 0.f;
constexpr float DEFAULT_GL_CLEAR_COLOR_GREEN         = 0.f;
constexpr float DEFAULT_GL_CLEAR_COLOR_BLUE          = 0.f;

constexpr float DEFAULT_CAMERA_MOVEMENT_SCALE_FACTOR = 1.f;
constexpr float DEFAULT_CAMERA_MOVEMENT_SPEED        = 7.5f;
constexpr float DEFAULT_CAMERA_MOUSE_SENSITIVITY     = 0.1f;
constexpr float DEFAULT_CAMERA_ROTATE_SENSITIVITY    = 0.006f;
constexpr float DEFAULT_CAMERA_DRAG_SENSITIVITY      = 0.05f;
constexpr float DEFAULT_CAMERA_WHEEL_SENSITIVITY     = 2.f;
constexpr float DEFAULT_CAMERA_ZOOM_SENSITIVITY      = 0.5f;

constexpr const char* GAME_PATH                      = "..\\Game";
constexpr const char* DEBUG_DLL_PATH                 = "..\\SobrassadaEngine\\x64\\Debug\\SobrassadaScripts.dll";
constexpr const char* RELEASE_DLL_PATH               = "..\\SobrassadaEngine\\x64\\Release\\SobrassadaScripts.dll";

constexpr const char* ENGINE_DEFAULT_ASSETS          = "EngineDefaults/";
constexpr const char* ASSETS_PATH                    = "Assets/";
constexpr const char* SCENES_PATH                    = "Assets/Scenes/";
constexpr const char* METADATA_PATH                  = "Assets/Metadata/";
constexpr const char* PREFABS_ASSETS_PATH            = "Assets/Prefabs/";
constexpr const char* MODELS_ASSETS_PATH             = "Assets/Models/";
constexpr const char* STATEMACHINES_ASSETS_PATH      = "Assets/StateMachines/";
constexpr const char* NAVMESH_ASSETS_PATH            = "Assets/Navmeshes/";

constexpr const char* LIBRARY_PATH                   = "Library/";
constexpr const char* ANIMATIONS_PATH                = "Library/Animations/";
constexpr const char* AUDIO_PATH                     = "Library/Audio/";
constexpr const char* MODELS_LIB_PATH                = "Library/Models/";
constexpr const char* MESHES_PATH                    = "Library/Meshes/";
constexpr const char* SCENES_PLAY_PATH               = "Library/Scenes/";
constexpr const char* TEXTURES_PATH                  = "Library/Textures/";
constexpr const char* MATERIALS_PATH                 = "Library/Materials/";
constexpr const char* STATEMACHINES_LIB_PATH         = "Library/StateMachines/";
constexpr const char* PREFABS_LIB_PATH               = "Library/Prefabs/";
constexpr const char* FONTS_PATH                     = "Library/Fonts/";
constexpr const char* NAVMESHES_PATH                 = "Library/Navmeshes/";

constexpr const char* ASSET_EXTENSION                = ".gltf";
constexpr const char* MESH_EXTENSION                 = ".sobrassada";
constexpr const char* TEXTURE_EXTENSION              = ".dds";
constexpr const char* MATERIAL_EXTENSION             = ".mat";
constexpr const char* SCENE_EXTENSION                = ".scene";
constexpr const char* PREFAB_EXTENSION               = ".prefab";
constexpr const char* MODEL_EXTENSION                = ".model";
constexpr const char* ANIMATION_EXTENSION            = ".anim";
constexpr const char* META_EXTENSION                 = ".smeta";
constexpr const char* STATEMACHINE_EXTENSION         = ".smachine";
constexpr const char* FONT_EXTENSION                 = ".ttf";
constexpr const char* NAVMESH_EXTENSION              = ".nav";

constexpr int MAX_COMPONENT_NAME_LENGTH              = 64;

// Soundbanks
constexpr const char* WINDOWS_BANKS_PATH             = "Assets\\Soundbanks\\Windows\\";
constexpr const wchar_t* BANKNAME_INIT               = L"Init.bnk";
constexpr const wchar_t* BANKNAME_MAIN               = L"main.bnk";
constexpr const char* BANKMETA_MAIN                  = "main.json";

// SHADER PATHS
constexpr const char* LIGHTS_VERTEX_SHADER_PATH      = "./EngineDefaults/Shader/Vertex/VertexShader.glsl";
constexpr const char* SKYBOX_VERTEX_SHADER_PATH      = "./EngineDefaults/Shader/Vertex/SkyboxVertex.glsl";
constexpr const char* UIWIDGET_VERTEX_SHADER_PATH    = "./EngineDefaults/Shader/Vertex/UIWidgetVertex.glsl";
constexpr const char* QUAD_VERTEX_SHADER_PATH        = "./EngineDefaults/Shader/Vertex/QuadVertexShader.glsl";
constexpr const char* UNLIT_FRAGMENT_SHADER_PATH     = "./EngineDefaults/Shader/Fragment/UnlitFragmentShader.glsl";
constexpr const char* SKYBOX_FRAGMENT_SHADER_PATH    = "./EngineDefaults/Shader/Fragment/SkyboxFragment.glsl";
constexpr const char* SPECULAR_FRAGMENT_SHADER_PATH  = "./EngineDefaults/Shader/Fragment/BRDFPhongFragmentShader.glsl";
constexpr const char* METALLIC_FRAGMENT_SHADER_PATH  = "./EngineDefaults/Shader/Fragment/BRDFCookTorranceShader.glsl";
constexpr const char* METALLIC_IBL_FRAGMENT_SHADER_PATH = "./EngineDefaults/Shader/Fragment/IBLShader.glsl";
constexpr const char* UIWIDGET_FRAGMENT_SHADER_PATH     = "./EngineDefaults/Shader/Fragment/UIWidgetFragment.glsl";
constexpr const char* IRRADIANCE_FRAGMENT_SHADER_PATH =
    "./EngineDefaults/Shader/Fragment/IrradianceCubemapFragment.glsl";
constexpr const char* PREFILTERED_FRAGMENT_SHADER_PATH =
    "./EngineDefaults/Shader/Fragment/PreFilteredEnvironmentFragment.glsl";
constexpr const char* ENVIRONMENTBRDF_FRAGMENT_SHADER_PATH =
    "./EngineDefaults/Shader/Fragment/EnvironmentBRDFFragment.glsl";
constexpr const char* GBUFFER_METALLIC_FRAGMENT_SHADER_PATH =
    "./EngineDefaults/Shader/Fragment/gBufferMetallicFragment.glsl";
constexpr const char* GBUFFER_SPECULAR_FRAGMENT_SHADER_PATH =
    "./EngineDefaults/Shader/Fragment/gBufferSpecularFragment.glsl";
constexpr const char* LIGHTINGPASS_FRAGMENT_SHADER_PATH  = "./EngineDefaults/Shader/Fragment/IBLLightingPass.glsl";

using UID                                                = uint64_t;

constexpr UID INVALID_UID                                = 0;
constexpr UID UID_PREFIX_DIVISOR                         = 100000000000000;
constexpr UID FALLBACK_TEXTURE_UID                       = 1200000000000000;
constexpr UID DEFAULT_MATERIAL_UID                       = 1300000000000000;

constexpr const char* CONSTANT_MESH_SELECT_DIALOG_ID     = "mesh-select";
constexpr const char* CONSTANT_MATERIAL_SELECT_DIALOG_ID = "material-select";
constexpr const char* CONSTANT_TEXTURE_SELECT_DIALOG_ID  = "texture-select";
constexpr const char* CONSTANT_DIFFUSE_TEXTURE_SELECT_DIALOG_ID  = "diffuse-texture-select";
constexpr const char* CONSTANT_METALLIC_TEXTURE_SELECT_DIALOG_ID = "metallic-texture-select";
constexpr const char* CONSTANT_SPECULAR_TEXTURE_SELECT_DIALOG_ID = "specular-texture-select";
constexpr const char* CONSTANT_NORMAL_TEXTURE_SELECT_DIALOG_ID   = "normal-texture-select";
constexpr const char* CONSTANT_PREFAB_SELECT_DIALOG_ID           = "prefab-select";
constexpr const char* CONSTANT_MODEL_SELECT_DIALOG_ID            = "model-select";
constexpr const char* CONSTANT_EVENT_SELECT_DIALOG_ID            = "event-select";

constexpr uint32_t CONSTANT_NO_MESH_UUID                         = 0;
constexpr uint32_t CONSTANT_NO_TEXTURE_UUID                      = 0;

constexpr float PI                                               = 3.14159265359f;
constexpr float RAD_DEGREE_CONV                                  = 180.f / PI;
constexpr float DEGREE_RAD_CONV                                  = PI / 180.f;

constexpr float MINIMUM_TREE_LEAF_SIZE                           = 5.f;
constexpr int PALETTE_SIZE                                       = 64;

inline UID GenerateUID()
{
    return static_cast<UID>(rng->IntFast()) << 32 | rng->IntFast(); // Combine two 32-bit values
}